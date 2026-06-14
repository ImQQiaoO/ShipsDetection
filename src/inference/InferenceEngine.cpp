#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "InferenceEngine.h"
#include "src/utils/Locale.hpp"
#include <algorithm>
#include <cstring>

InferenceEngine::InferenceEngine(Ort::Session *session, ModelInit &mod,
                                 float conf_threshold, float nms_threshold)
    : session_(session), mod_(mod),
      conf_threshold_(conf_threshold), nms_threshold_(nms_threshold),
      scale_(1.0f), pad_w_(0), pad_h_(0), orig_w_(0), orig_h_(0),
      memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU)) {

    // Cache input/output names
    input_name_ = mod_.get_input_name();
    output_name_strings_ = mod_.get_output_names();
    output_names_cstr_.reserve(output_name_strings_.size());
    for (const auto &name : output_name_strings_) {
        output_names_cstr_.push_back(name.c_str());
    }

    // Pre-allocate tensor buffer (1 * 3 * 640 * 640)
    constexpr int tensor_size = 3 * ModelInit::INPUT_WIDTH * ModelInit::INPUT_HEIGHT;
    input_tensor_values_.resize(tensor_size);

    // Pre-allocate transposed output buffer
    // YOLOv8 output: [1, 84, 8400] → transposed: [8400, 84]
    // 84 = 4 (bbox) + 80 (classes), 8400 = num_boxes for 640x640 input
    constexpr int max_boxes = 8400;
    constexpr int max_dims = 84;
    transposed_output_.resize(max_boxes * max_dims);

    // Reserve capacity for detection vectors
    boxes_.reserve(64);
    confidences_.reserve(64);
    class_ids_.reserve(64);
    indices_.reserve(64);

    // Create IOBinding
    io_binding_ = std::make_unique<Ort::IoBinding>(*session_);
}

std::vector<DetectionResult> InferenceEngine::detect(cv::Mat &frame) {
    preprocess(frame);
    run_inference();
    postprocess();

    // Draw bounding boxes on frame
    drawer_.draw(frame, indices_, boxes_, confidences_, class_ids_);

    // Build result vector
    std::vector<DetectionResult> results;
    results.reserve(indices_.size());
    for (const int i : indices_) {
        results.push_back({
            std::to_string(class_ids_[i]),
            confidences_[i],
            boxes_[i]
        });
    }
    return results;
}

std::vector<DetectionResult> InferenceEngine::detect_no_draw(cv::Mat &frame) {
    preprocess(frame);
    run_inference();
    postprocess();

    std::vector<DetectionResult> results;
    results.reserve(indices_.size());
    for (const int i : indices_) {
        results.push_back({
            std::to_string(class_ids_[i]),
            confidences_[i],
            boxes_[i]
        });
    }
    return results;
}

// ============================================================
// 融合预处理：一次遍历完成 BGR→RGB + /255 归一化 + HWC→CHW 布局变换
// 替代原来的 convertTo + cvtColor + split 三次遍历
// ============================================================
void InferenceEngine::preprocess(const cv::Mat &frame) {
    orig_w_ = frame.cols;
    orig_h_ = frame.rows;

    // Letterbox resize (maintains aspect ratio)
    constexpr int target_w = ModelInit::INPUT_WIDTH;
    constexpr int target_h = ModelInit::INPUT_HEIGHT;

    scale_ = std::min(static_cast<float>(target_w) / static_cast<float>(orig_w_),
                      static_cast<float>(target_h) / static_cast<float>(orig_h_));

    int new_unpad_w = static_cast<int>(std::round(static_cast<float>(orig_w_) * scale_));
    int new_unpad_h = static_cast<int>(std::round(static_cast<float>(orig_h_) * scale_));

    pad_w_ = (target_w - new_unpad_w) / 2;
    pad_h_ = (target_h - new_unpad_h) / 2;

    cv::resize(frame, resized_, cv::Size(new_unpad_w, new_unpad_h));
    cv::copyMakeBorder(resized_, padded_, pad_h_, target_h - new_unpad_h - pad_h_,
                       pad_w_, target_w - new_unpad_w - pad_w_,
                       cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));

    // 融合：BGR uint8 → RGB float32 CHW，一次遍历
    const int img_size = target_w * target_h;
    float *r_ptr = input_tensor_values_.data();
    float *g_ptr = r_ptr + img_size;
    float *b_ptr = g_ptr + img_size;

    constexpr float scale_factor = 1.0f / 255.0f;
    const int total_pixels = img_size;
    const uchar *src = padded_.data;

    for (int i = 0; i < total_pixels; ++i) {
        // OpenCV BGR layout: B, G, R per pixel
        b_ptr[i] = static_cast<float>(src[i * 3 + 0]) * scale_factor;
        g_ptr[i] = static_cast<float>(src[i * 3 + 1]) * scale_factor;
        r_ptr[i] = static_cast<float>(src[i * 3 + 2]) * scale_factor;
    }
}

// ============================================================
// 使用IOBinding执行推理，转置输出以改善后处理缓存命中
// ============================================================
void InferenceEngine::run_inference() {
    auto input_name_cstr = input_name_.c_str();

    // 创建输入张量（指向预分配缓冲区）
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info_,
        input_tensor_values_.data(), input_tensor_values_.size(),
        ModelInit::input_shape.data(), ModelInit::input_shape.size());

    // 使用IOBinding避免输出张量重复分配
    io_binding_->ClearBoundInputs();
    io_binding_->ClearBoundOutputs();
    io_binding_->BindInput(input_name_cstr, input_tensor);
    // 让 ORT 分配输出到 GPU 侧，再取回（对 CUDA EP 有效）
    io_binding_->BindOutput(output_names_cstr_[0], memory_info_);

    session_->Run(Ort::RunOptions{nullptr}, *io_binding_);

    // 获取输出
    std::vector<Ort::Value> output_tensors = io_binding_->GetOutputValues();
    float *output_data = output_tensors[0].GetTensorMutableData<float>();
    auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

    int64_t dimensions = output_shape[1];  // 84 (4 bbox + num_classes)
    int64_t num_boxes = output_shape[2];   // 8400

    // 转置 [1, dimensions, num_boxes] → [num_boxes, dimensions]
    // 使得后续逐框访问是连续内存，极大改善缓存命中率
    for (int64_t j = 0; j < dimensions; ++j) {
        const float *src_row = output_data + j * num_boxes;
        for (int64_t i = 0; i < num_boxes; ++i) {
            transposed_output_[i * dimensions + j] = src_row[i];
        }
    }

    // Clear reusable vectors (capacity preserved)
    boxes_.clear();
    confidences_.clear();
    class_ids_.clear();

    // 后处理：现在逐框顺序访问内存是连续的
    for (int64_t i = 0; i < num_boxes; ++i) {
        const float *row = transposed_output_.data() + i * dimensions;

        // Find max class score (从第4个元素开始)
        float max_class_score = 0;
        int class_id = -1;
        for (int64_t j = 4; j < dimensions; ++j) {
            float score = row[j];
            if (score > max_class_score) {
                max_class_score = score;
                class_id = static_cast<int>(j - 4);
            }
        }
        if (max_class_score < conf_threshold_)
            continue;

        // Extract bbox (center_x, center_y, w, h)
        float x = row[0];
        float y = row[1];
        float w = row[2];
        float h = row[3];

        // Map back to original image coordinates
        float x_center = (x - static_cast<float>(pad_w_)) / scale_;
        float y_center = (y - static_cast<float>(pad_h_)) / scale_;
        float width = w / scale_;
        float height = h / scale_;

        int left = std::max(0, std::min(static_cast<int>(x_center - width / 2), orig_w_ - 1));
        int top = std::max(0, std::min(static_cast<int>(y_center - height / 2), orig_h_ - 1));
        int box_w = std::min(static_cast<int>(width), orig_w_ - left);
        int box_h = std::min(static_cast<int>(height), orig_h_ - top);

        boxes_.emplace_back(left, top, box_w, box_h);
        confidences_.push_back(max_class_score);
        class_ids_.push_back(class_id);
    }
}

void InferenceEngine::postprocess() {
    cv::dnn::NMSBoxes(boxes_, confidences_, conf_threshold_, nms_threshold_, indices_);
}

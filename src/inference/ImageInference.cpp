#include <onnxruntime_cxx_api.h>
#include "ImageInference.h"
#include "DrawBoundingBox.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <opencv2/opencv.hpp>
#include <cuda_runtime.h>

/**
 * 这个函数的目的是将任意尺寸的输入图像调整为模型所需的固定尺寸（如640×640），
 * 同时保持图像的原始宽高比以避免失真，并用灰色填充多余的空间。
 * @param img       输入图像
 * @param new_shape 目标尺寸
 * @param scale     输出参数，记录实际缩放比例
 * @param pad_w     输出参数，记录填充的像素数
 * @param pad_h     输出参数，记录填充的像素数
 */
cv::Mat ImageInference::letterbox(const cv::Mat &img, const cv::Size &new_shape, float &scale, int &pad_w, int &pad_h) {
    int width = img.cols;
    int height = img.rows;
    // 计算缩放比例：取宽高的最小比例
    scale = std::min(static_cast<float>(new_shape.width) / static_cast<float>(width),
        static_cast<float>(new_shape.height) / static_cast<float>(height));
    int new_unpad_w = static_cast<int>(std::round(static_cast<float>(width) * scale));
    int new_unpad_h = static_cast<int>(std::round(static_cast<float>(height) * scale));
    // 计算需要填充的宽高（左右和上下分别填充一半）
    pad_w = new_shape.width - new_unpad_w;
    pad_h = new_shape.height - new_unpad_h;
    int pad_w_half = pad_w / 2;
    int pad_h_half = pad_h / 2;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_unpad_w, new_unpad_h));
    // 添加边框，默认填充值为114（灰色）
    cv::Mat out;
    cv::copyMakeBorder(resized, out, pad_h_half, pad_h - pad_h_half,
        pad_w_half, pad_w - pad_w_half,
        cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    // 返回填充后的图像，同时更新填充的像素数（左上角的pad）
    pad_w = pad_w_half;
    pad_h = pad_h_half;
    return out;
}

// 调试函数：打印 tensor 信息
void ImageInference::print_tensor_info(const Ort::Value &tensor, const char *name) {
    const auto type_info = tensor.GetTensorTypeAndShapeInfo();
    const auto shape = type_info.GetShape();

    std::cout << "Tensor name: " << name << '\n';
    std::cout << "Shape: [";
    for (size_t i = 0; i < shape.size(); i++) {
        std::cout << shape[i];
        if (i < shape.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << '\n';
}

// 构造函数
ImageInference::ImageInference(cv::Mat img, Ort::Session *session, ModelInit &mod) : img_(std::move(img)), session_(session) {
    // 保存原图尺寸
    orig_w_ = img_.cols;
    orig_h_ = img_.rows;
#if (!defined(NDEBUG))
    std::cout << "原图尺寸: " << orig_w_ << "x" << orig_h_ << '\n';
#endif
    img_letterbox_ = preprocess_image();                // STEP1: 图像预处理（包括 letterbox）
    convert_to_tensor();                                // STEP2: 转换为tensor格式
    Ort::Value output_tensor = run_inference(mod);   // STEP3: 推理计算
    process_output(output_tensor);                   // STEP4: 处理推理结果（后处理）
    NMS_boxes();                                        // STEP5: 非极大值抑制
}

cv::Mat ImageInference::preprocess_image() {
    // 使用 letterbox 调整图片大小
    cv::Mat img_lb = letterbox(img_, cv::Size(ModelInit::INPUT_WIDTH, ModelInit::INPUT_HEIGHT), scale_, pad_w_, pad_h_);
#if (!defined(NDEBUG))
    std::cout << "Letterbox信息: scale=" << scale_ << ", pad_w=" << pad_w_ << ", pad_h=" << pad_h_ << '\n';
#endif
    // 预处理：归一化、BGR -> RGB
    cv::Mat blob;
    img_lb.convertTo(blob, CV_32F, 1.0 / 255.0);
    cv::cvtColor(blob, blob, cv::COLOR_BGR2RGB);

    return blob; // 这里返回的blob仍为 HWC 排列
}

void ImageInference::convert_to_tensor() {
    const int height = ModelInit::INPUT_HEIGHT;
    const int width = ModelInit::INPUT_WIDTH;
    const int img_size = height * width;

    input_tensor_values_.resize(1 * 3 * img_size);

    // 预先计算偏移量，避免重复计算
    float *r_channel = input_tensor_values_.data();
    float *g_channel = r_channel + img_size;
    float *b_channel = g_channel + img_size;

    // 单次遍历
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            const cv::Vec3f &pixel = img_letterbox_.at<cv::Vec3f>(h, w);
            int offset = h * width + w;

            r_channel[offset] = pixel[0];
            g_channel[offset] = pixel[1];
            b_channel[offset] = pixel[2];
        }
    }
}

Ort::Value ImageInference::run_inference(ModelInit &mod) {
    // 计算输入数据的字节数
    size_t num_elements = input_tensor_values_.size();
    size_t byte_size = num_elements * sizeof(float);

    // 1. 在GPU上分配内存
    float *gpu_data = nullptr;
    cudaError_t cudaStatus = cudaMalloc(reinterpret_cast<void **>(&gpu_data), byte_size);
    if (cudaStatus != cudaSuccess) {
        throw std::runtime_error("cudaMalloc failed");
    }

    // 2. 将CPU数据复制到GPU内存中
    cudaStatus = cudaMemcpy(gpu_data, input_tensor_values_.data(), byte_size, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        cudaFree(gpu_data);  // 出错时先释放已分配的内存
        throw std::runtime_error("cudaMemcpy failed");
    }

    // 3. 使用CUDA创建MemoryInfo，device_id为0，请根据实际情况修改
    Ort::MemoryInfo memory_info("Cuda", OrtAllocatorType::OrtArenaAllocator, 0, OrtMemTypeDefault);

    // 利用GPU内存中的数据创建输入张量
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        gpu_data, num_elements,
        ModelInit::input_shape.data(), ModelInit::input_shape.size()
    );

    // 4. 准备输入和输出的名称
    auto input_name = mod.get_input_name().c_str();
    std::vector<const char *> output_names;
    for (const auto &str : mod.get_output_names()) {
        output_names.push_back(str.c_str());
    }

    // 5. 运行模型推理
    std::vector<Ort::Value> output_tensors = session_->Run(
        Ort::RunOptions {nullptr},
        &input_name, &input_tensor, 1,
        output_names.data(), output_names.size()
    );

#if (!defined(NDEBUG))
    // 调试：打印输出张量信息
    for (size_t i = 0; i < output_tensors.size(); i++) {
        print_tensor_info(output_tensors[i], output_names[i]);
    }
#endif

    // 6. 推理完成后释放GPU内存
    cudaFree(gpu_data);

    // 返回第一个输出张量
    return std::move(output_tensors[0]);
}

//Ort::Value ImageInference::run_inference(ModelInit &mod) {
//    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
//
//    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
//        memory_info,
//        input_tensor_values_.data(), input_tensor_values_.size(),
//        ModelInit::input_shape.data(), ModelInit::input_shape.size());
//
//    auto input_name = mod.get_input_name().c_str();
//    std::vector<const char *> output_names;
//    for (const auto &str : mod.get_output_names()) {
//        output_names.push_back(str.c_str());
//    }
//
//    std::vector<Ort::Value> output_tensors = session_->Run(
//        Ort::RunOptions {nullptr},
//        &input_name, &input_tensor, 1,
//        output_names.data(), output_names.size());
//
//#if (!defined(NDEBUG))
//    // 调试：打印输出张量信息
//    for (size_t i = 0; i < output_tensors.size(); i++) {
//        print_tensor_info(output_tensors[i], output_names[i]);
//    }
//#endif
//
//    return std::move(output_tensors[0]);  // 返回第一个输出张量
//}

void ImageInference::process_output(Ort::Value &output_tensor, float conf_threshold) {
    float *output_data = output_tensor.GetTensorMutableData<float>();
    auto output_shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();

    // 获取输出维度和检测框数量
    int64_t dimensions = output_shape[1];
    int64_t num_boxes = output_shape[2];
#if (!defined(NDEBUG))
    std::cout << "输出维度: " << dimensions << ", 框数量: " << num_boxes << '\n';
#endif
    // 遍历每个检测框并提取有效候选框
    for (int i = 0; i < num_boxes; ++i) {
        float max_class_score = 0;
        int class_id = -1;
        for (int j = 4; j < dimensions; ++j) {
            float class_score = output_data[j * num_boxes + i];
            if (class_score > max_class_score) {
                max_class_score = class_score;
                class_id = j - 4;
            }
        }
        if (max_class_score < conf_threshold)
            continue;

        // 提取边界框数据（YOLO输出格式：中心点 + 宽高）
        float x = output_data[0 * num_boxes + i];
        float y = output_data[1 * num_boxes + i];
        float w = output_data[2 * num_boxes + i];
        float h = output_data[3 * num_boxes + i];
#if (!defined(NDEBUG))
        std::cout << "原始检测结果: x=" << x << ", y=" << y << ", w=" << w << ", h=" << h
            << ", class=" << class_id << ", score=" << max_class_score << '\n';
#endif
        // 将 letterbox 坐标映射回原图
        float x_center = (x - static_cast<float>(pad_w_)) / scale_;
        float y_center = (y - static_cast<float>(pad_h_)) / scale_;
        float width = w / scale_;
        float height = h / scale_;
        int left = static_cast<int>(x_center - width / 2);
        int top = static_cast<int>(y_center - height / 2);

        // 边界检查
        left = std::max(0, std::min(left, orig_w_ - 1));
        top = std::max(0, std::min(top, orig_h_ - 1));
        int box_w = std::min(static_cast<int>(width), orig_w_ - left);
        int box_h = std::min(static_cast<int>(height), orig_h_ - top);

        boxes_.emplace_back(left, top, box_w, box_h);
        confidences_.push_back(max_class_score);
        class_ids_.push_back(class_id);
#if (!defined(NDEBUG))
        std::cout << "转换后: left=" << left << ", top=" << top << ", width=" << box_w << ", height=" << box_h << '\n';
#endif

    }
}

void ImageInference::NMS_boxes(float conf_threshold, float nms_threshold) {
    cv::dnn::NMSBoxes(boxes_, confidences_, conf_threshold, nms_threshold, indices_);
}

void ImageInference::draw_bounding_box() {
    // 绘制检测框
    DrawBoundingBox bounding_box;
    bounding_box.draw(img_, indices_, boxes_, confidences_, class_ids_);
}

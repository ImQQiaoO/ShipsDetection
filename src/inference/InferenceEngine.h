#pragma once
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include "ModelInit.h"
#include "DrawBoundingBox.h"
#include "ImageInference.h"

/**
 * 可复用的推理引擎，预分配所有缓冲区，避免逐帧内存分配。
 * 优化：融合预处理、转置后处理、IOBinding。
 */
class InferenceEngine {
public:
    InferenceEngine(Ort::Session *session, ModelInit &mod,
                    float conf_threshold = 0.48f, float nms_threshold = 0.45f);

    /// 对输入帧执行检测并在帧上绘制结果，返回检测信息
    std::vector<DetectionResult> detect(cv::Mat &frame);

    /// 仅执行推理，不绘制，返回检测结果
    std::vector<DetectionResult> detect_no_draw(cv::Mat &frame);

private:
    // Session & model (non-owning)
    Ort::Session *session_;
    ModelInit &mod_;

    // Thresholds
    float conf_threshold_;
    float nms_threshold_;

    // Pre-cached output names (c_str pointers valid for engine lifetime)
    std::string input_name_;
    std::vector<std::string> output_name_strings_;
    std::vector<const char *> output_names_cstr_;

    // Pre-allocated buffers (reused across frames)
    std::vector<float> input_tensor_values_;
    std::vector<float> transposed_output_;  // 转置后的输出缓冲区 [num_boxes, dimensions]
    std::vector<cv::Rect> boxes_;
    std::vector<float> confidences_;
    std::vector<int> class_ids_;
    std::vector<int> indices_;

    // Letterbox metadata (updated per frame)
    float scale_;
    int pad_w_;
    int pad_h_;
    int orig_w_;
    int orig_h_;

    // Drawing (constructed once, YAML parsed once)
    DrawBoundingBox drawer_;

    // IOBinding for pre-allocated output
    std::unique_ptr<Ort::IoBinding> io_binding_;
    Ort::MemoryInfo memory_info_;

    // Internal pipeline steps
    void preprocess(const cv::Mat &frame);
    void run_inference();
    void postprocess();

    // Reusable intermediate
    cv::Mat resized_;
    cv::Mat padded_;  // letterbox结果 (uint8)
};

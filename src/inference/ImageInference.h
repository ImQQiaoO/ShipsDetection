#pragma once
#include <opencv2/opencv.hpp>
#include "ModelInit.h"

struct DetectionResult {
    std::string class_name;
    float confidence;
    cv::Rect bbox;
};

class ImageInference {
public:
    ImageInference(cv::Mat img, Ort::Session *session, ModelInit &mod);
    void draw_bounding_box();
    std::vector<DetectionResult> get_curr_info() const;
private:
    // 原始输入图像、推理模型的会话指针
    cv::Mat img_;
    Ort::Session *session_;
    // 中间处理信息
    int orig_w_;
    int orig_h_;
    float scale_;
    int pad_w_;
    int pad_h_;
    cv::Mat img_letterbox_;
    std::vector<float> input_tensor_values_;

    // 检测结果（推理输出的后处理结果）
    std::vector<cv::Rect> boxes_;
    std::vector<float> confidences_;
    std::vector<int> class_ids_;

    std::vector<int> indices_; // For NMS

    // Allocate GPU Memory
    float *gpu_data_;  // GPU内存指针
    bool gpu_allocated_;  // 标记是否已分配GPU内存

    // 私有成员函数：每个步骤各自封装
    cv::Mat preprocess_image();
    void convert_to_tensor();
    Ort::Value run_inference(ModelInit &mod);
    void process_output(Ort::Value &output_tensor, float conf_threshold = 0.48f);
    void NMS_boxes(float conf_threshold = 0.48f, float nms_threshold = 0.45f);

    // 工具函数
    static cv::Mat letterbox(const cv::Mat &img, const cv::Size &new_shape, float &scale, int &pad_w, int &pad_h);
    static void print_tensor_info(const Ort::Value &tensor, const char *name);
};


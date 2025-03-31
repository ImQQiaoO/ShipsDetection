#include <iostream>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
const wchar_t *char_to_wchar(const char *charStr) {
    if (charStr == nullptr) return nullptr;
    int size = MultiByteToWideChar(CP_UTF8, 0, charStr, -1, nullptr, 0);
    if (size == 0) return nullptr;
    wchar_t *wcharStr = new wchar_t[size];
    int result = MultiByteToWideChar(CP_UTF8, 0, charStr, -1, wcharStr, size);
    if (result == 0) {
        delete[] wcharStr;
        return nullptr;
    }
    return wcharStr;
}
#endif

// letterbox函数：将图片resize到指定尺寸（如640x640）并保持宽高比，用灰色填充空白区域
cv::Mat letterbox(const cv::Mat &img, const cv::Size &new_shape, float &scale, int &pad_w, int &pad_h) {
    int width = img.cols;
    int height = img.rows;
    // 计算缩放比例：取宽高的最小比例
    scale = std::min(new_shape.width / (float)width, new_shape.height / (float)height);
    int new_unpad_w = std::round(width * scale);
    int new_unpad_h = std::round(height * scale);
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
void printTensorInfo(const Ort::Value &tensor, const char *name) {
    auto typeInfo = tensor.GetTensorTypeAndShapeInfo();
    auto shape = typeInfo.GetShape();

    std::cout << "Tensor name: " << name << '\n';
    std::cout << "Shape: [";
    for (size_t i = 0; i < shape.size(); i++) {
        std::cout << shape[i];
        if (i < shape.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << '\n';
}

int main() {
    // 初始化 onnxruntime 环境和会话
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "YOLO");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    std::string modelPath = "./runs/detect/train4/weights/best.onnx";

#ifdef _WIN32
    Ort::Session session(env, char_to_wchar(modelPath.c_str()), session_options);
#else
    Ort::Session session(env, modelPath.c_str(), session_options);
#endif

    Ort::AllocatorWithDefaultOptions allocator;

    // 获取输入节点名称
    auto input_name_alloc = session.GetInputNameAllocated(0, allocator);
    const char *input_name = input_name_alloc.get();
    std::cout << "输入名称: " << input_name << '\n';

    // 获取输出节点名称
    size_t outputCount = session.GetOutputCount();
    std::vector<const char *> outputNames;
    std::vector<std::string> outputNameStr; // 保存字符串，避免内存泄漏

    for (size_t i = 0; i < outputCount; i++) {
        auto output_name_alloc = session.GetOutputNameAllocated(i, allocator);
        outputNameStr.emplace_back(output_name_alloc.get());
        outputNames.push_back(outputNameStr.back().c_str());
        std::cout << "输出名称 " << i << ": " << outputNameStr.back() << '\n';
    }

    // 模型输入尺寸
    std::vector<int64_t> inputShape = {1, 3, 640, 640};
    const int INPUT_WIDTH = 640;
    const int INPUT_HEIGHT = 640;

    std::string targetDir = "./target/";
    for (const auto &entry : fs::directory_iterator(targetDir)) {
        std::string imagePath = entry.path().string();
        std::cout << "处理图片: " << imagePath << '\n';

        cv::Mat img = cv::imread(imagePath);
        if (img.empty()) {
            std::cerr << "无法读取图片: " << imagePath << '\n';
            continue;
        }

        // 保存原图尺寸，用于后续坐标映射
        int orig_w = img.cols;
        int orig_h = img.rows;
        std::cout << "原图尺寸: " << orig_w << "x" << orig_h << '\n';

        // 使用 letterbox 保持比例调整图片到640x640
        float scale;
        int pad_w, pad_h;
        cv::Mat img_letterbox = letterbox(img, cv::Size(INPUT_WIDTH, INPUT_HEIGHT), scale, pad_w, pad_h);

        std::cout << "Letterbox信息: scale=" << scale << ", pad_w=" << pad_w << ", pad_h=" << pad_h << '\n';

        // 预处理：归一化、BGR转RGB，并转换为 float 类型
        cv::Mat blob;
        img_letterbox.convertTo(blob, CV_32F, 1.0 / 255.0);
        cv::cvtColor(blob, blob, cv::COLOR_BGR2RGB);

        // HWC转CHW并放入vector中
        std::vector<float> inputTensorValues(1 * 3 * INPUT_WIDTH * INPUT_HEIGHT);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < INPUT_HEIGHT; ++h) {
                for (int w = 0; w < INPUT_WIDTH; ++w) {
                    inputTensorValues[c * INPUT_WIDTH * INPUT_HEIGHT + h * INPUT_WIDTH + w] =
                        blob.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        // 创建输入tensor
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo,
            inputTensorValues.data(), inputTensorValues.size(),
            inputShape.data(), inputShape.size());

        // 执行推理
        auto outputTensors = session.Run(Ort::RunOptions {nullptr},
            &input_name, &inputTensor, 1,
            outputNames.data(), outputNames.size());

        // 打印输出tensor信息以便调试
        for (size_t i = 0; i < outputTensors.size(); i++) {
            printTensorInfo(outputTensors[i], outputNames[i]);
        }

        // 获取输出数据
        float *outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        float confThreshold = 0.25f;
        float nmsThreshold = 0.45f;
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        // YOLOv8的输出通常是 [1, 84, 8400] 或者 [1, NUM_CLASSES+4, NUM_BOXES]
        // 注意YOLOv8输出格式：前4个值是边界框坐标(x,y,w,h)，后面是类别置信度
        int dimensions = outputShape[1]; // 类别数+4(坐标)
        int num_boxes = outputShape[2];  // 检测到的框数量

        std::cout << "输出维度: " << dimensions << ", 框数量: " << num_boxes << '\n';

        // 处理每一个检测框
        for (int i = 0; i < num_boxes; ++i) {
            // 寻找最高置信度的类别
            float max_class_score = 0;
            int class_id = -1;

            // 从下标4开始是类别置信度
            for (int j = 4; j < dimensions; ++j) {
                float class_score = outputData[j * num_boxes + i];
                if (class_score > max_class_score) {
                    max_class_score = class_score;
                    class_id = j - 4;
                }
            }

            if (max_class_score < confThreshold)
                continue;

            // 获取边界框坐标 (YOLO输出是xywh格式，中心点+宽高)
            float x = outputData[0 * num_boxes + i]; // center_x
            float y = outputData[1 * num_boxes + i]; // center_y
            float w = outputData[2 * num_boxes + i]; // width
            float h = outputData[3 * num_boxes + i]; // height

            // 打印原始检测结果
            std::cout << "原始检测结果: x=" << x << ", y=" << y << ", w=" << w << ", h=" << h
                << ", class=" << class_id << ", score=" << max_class_score << '\n';

            // 从letterbox坐标映射回原图坐标
            float x_center = (x - pad_w) / scale;
            float y_center = (y - pad_h) / scale;
            float width = w / scale;
            float height = h / scale;

            // 转换为左上角坐标
            int left = static_cast<int>(x_center - width / 2);
            int top = static_cast<int>(y_center - height / 2);

            // 边界检查
            left = std::max(0, std::min(left, orig_w - 1));
            top = std::max(0, std::min(top, orig_h - 1));
            int box_w = std::min(static_cast<int>(width), orig_w - left);
            int box_h = std::min(static_cast<int>(height), orig_h - top);

            boxes.emplace_back(left, top, box_w, box_h);
            confidences.push_back(max_class_score);
            classIds.push_back(class_id);

            std::cout << "转换后: left=" << left << ", top=" << top << ", width=" << box_w << ", height=" << box_h << std::endl;
        }

        // 非极大值抑制（NMS）
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

        // 绘制检测框
        const char *class_names[] = {"containership", "cruise", "bulkcarrier", "other", "fishing"};
        for (int idx : indices) {
            cv::Rect box = boxes[idx];
            float conf = confidences[idx];
            int class_id = classIds[idx];

            std::cout << "检测框: x=" << box.x << ", y=" << box.y
                << ", width=" << box.width << ", height=" << box.height
                << ", class=" << class_id << ", conf=" << conf << '\n';

            // 绘制框和标签
            cv::rectangle(img, box, cv::Scalar(0, 255, 0), 2);

            // 添加标签
            std::string label = class_id < 5 ? class_names[class_id] : "unknown";
            label += " " + std::to_string(static_cast<int>(conf * 100)) + "%";

            int baseline = 0;
            cv::Size label_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
            cv::rectangle(img, cv::Point(box.x, box.y - label_size.height - baseline - 10),
                cv::Point(box.x + label_size.width, box.y), cv::Scalar(0, 255, 0), cv::FILLED);
            cv::putText(img, label, cv::Point(box.x, box.y - baseline - 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
        }

        // 保存和显示结果
        cv::imwrite("result_" + entry.path().filename().string(), img);
        cv::imshow("检测结果", img);
        cv::waitKey(0);
    }

    return 0;
}


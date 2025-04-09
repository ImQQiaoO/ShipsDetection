#include "inference/DrawBoundingBox.h"
#include "inference/SessionManager.h"
#include "inference/ModelInit.h"
#include "inference/ImageInference.h"
#include <filesystem>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

int main() {
    std::string model_path = "./runs/detect/train4/weights/best.onnx";
    SessionManager session_manager(model_path);
    Ort::Session *session = session_manager.get_session();

    ModelInit mod(session);

    std::vector<const char *> output_names;
    for (const auto &str : mod.get_output_names()) {
        output_names.push_back(str.c_str());
    }

    std::string target_dir = "./target/";
    for (const auto &entry : fs::directory_iterator(target_dir)) {
        std::string image_path = entry.path().string();
        std::cout << "处理图片: " << image_path << '\n';
        cv::Mat img = cv::imread(image_path);
        if (img.empty()) {
            std::cerr << "无法读取图片: " << image_path << '\n';
            continue;
        }
        ImageInference image_inference(img, session, mod);
        image_inference.draw_bounding_box();

        std::string window_name = "检测结果 - " + entry.path().filename().string();
        cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
        cv::imshow(window_name, img);
        cv::waitKey(1);
    }
    cv::waitKey(0);

    return 0;
}


#include "inference/SessionManager.h"
#include "inference/ModelInit.h"
#include "inference/ImageInference.h"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "src/utils/Locale.hpp"
#include <QApplication>
#include "src/ui/MainPanel.h"

namespace fs = std::filesystem;

void reg_video(Ort::Session *session, ModelInit &mod) {
    std::string video_path = "./target_video/ship_video.mp4";
    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        utils::utf2ansi_out << "无法打开视频文件: " << video_path << '\n';
        return;
    }

    double input_fps = cap.get(cv::CAP_PROP_FPS);
    utils::utf2ansi_out << "视频输入帧率: " << input_fps << " FPS" << '\n';

    cv::Mat frame;
    int frame_count = 0;
    auto overall_start = std::chrono::high_resolution_clock::now();

    while (cap.read(frame)) {
        // 处理推理和绘制边界框
        ImageInference image_inference(frame, session, mod);
        image_inference.draw_bounding_box();

        cv::imshow("Video Frame", frame);
        ++frame_count;

        if (cv::waitKey(1) >= 0)
            break;
    }

    auto overall_end = std::chrono::high_resolution_clock::now();
    double total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start).count();
    double average_fps = (frame_count > 0) ? (frame_count * 1000.0 / total_time_ms) : 0;

    utils::utf2ansi_out << "处理了 " << frame_count << " 帧, 总耗时："
        << total_time_ms << " ms, 平均帧率：" << average_fps << " FPS" << '\n';

    cap.release();
    cv::destroyAllWindows();
}

// 识别整个文件夹的
void reg_img(const fs::path &image_path, Ort::Session *session, ModelInit &mod) {
    utils::utf2ansi_out << "处理图片: " << image_path << '\n';
    cv::Mat img = cv::imread(image_path.string());
    if (img.empty()) {
        std::cerr << "无法读取图片: " << image_path << '\n';
        return;
    }
    ImageInference image_inference(img, session, mod);
    image_inference.draw_bounding_box();

    std::string window_name = utils::utf8_to_ansi("检测结果 - " + image_path.filename().string());
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
    cv::imshow(window_name, img);
    cv::waitKey(1);
}

int main(int argc, char *argv[]) {
    std::string model_path = "./runs/detect/train4/weights/best.onnx";
    SessionManager session_manager(model_path);
    Ort::Session *session = session_manager.get_session();

    ModelInit mod(session);

    //std::vector<const char *> output_names;
    //for (const auto &str : mod.get_output_names()) {
    //    output_names.push_back(str.c_str());
    //}

    //std::string target_dir = "./target/";
    //for (const auto &entry : fs::directory_iterator(target_dir)) {
    //    reg_img(entry.path(), session, mod);
    //}
    //cv::waitKey(0);

    //reg_video(session, mod);

    QApplication app(argc, argv);

    // 创建并显示视频播放器
    MainPanel panel(session, mod);
    panel.show();

    return QApplication::exec();
    
}


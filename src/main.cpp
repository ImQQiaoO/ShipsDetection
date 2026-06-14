#include "inference/SessionManager.h"
#include "inference/ModelInit.h"
#include "inference/InferenceEngine.h"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "src/utils/Locale.hpp"
#include <QApplication>
#include "src/ui/MainPanel.h"

namespace fs = std::filesystem;

void reg_video(InferenceEngine &engine) {
    std::string video_path = "./target_video/ship_video.mp4";
    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        utils::utf2ansi_out << "无法打开视频文件: " << video_path << '\n';
        return;
    }

    double input_fps = cap.get(cv::CAP_PROP_FPS);
    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    utils::utf2ansi_out << "视频输入帧率: " << input_fps << " FPS, 分辨率: " << width << "x" << height << '\n';

    cv::Mat frame;
    int frame_count = 0;

    // 预热：前几帧不计入统计（CUDA/cuDNN 首次执行有额外初始化开销）
    int warmup_frames = 5;
    while (warmup_frames > 0 && cap.read(frame)) {
        engine.detect(frame);
        --warmup_frames;
    }

    auto overall_start = std::chrono::high_resolution_clock::now();

    while (cap.read(frame)) {
        engine.detect(frame);
        ++frame_count;

        cv::imshow("Video Frame", frame);
        if (cv::waitKey(1) >= 0)
            break;
    }

    auto overall_end = std::chrono::high_resolution_clock::now();
    double total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(overall_end - overall_start).count();
    double average_fps = (frame_count > 0) ? (frame_count * 1000.0 / total_time_ms) : 0;

    utils::utf2ansi_out << "处理了 " << frame_count << " 帧 (预热 5 帧未计入), 总耗时："
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

    //// 保存识别后的图像
    //std::string output_path = image_path.stem().string() + "_result" + image_path.extension().string();
    //if (!cv::imwrite(output_path, img)) {
    //    utils::utf2ansi_out << "无法保存图像: " << output_path << '\n';
    //} else {
    //    utils::utf2ansi_out << "图像已保存: " << output_path << '\n';
    //}
}

int main(int argc, char *argv[]) {
    std::string model_path = "./runs/detect/train4/weights/best.onnx";
    SessionManager session_manager(model_path);
    Ort::Session *session = session_manager.get_session();

    ModelInit mod(session);
    InferenceEngine engine(session, mod);

    reg_video(engine);

    //QApplication app(argc, argv);
    //MainPanel panel(session, mod);
    //panel.show();
    //return QApplication::exec();
    return 0;
}

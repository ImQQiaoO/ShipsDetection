#include "MediaPlayer.h"
#include "src/inference/ImageInference.h"
#include "src/utils/Locale.hpp"
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QPixmapCache>
#include <QFileDialog>

MediaPlayer::MediaPlayer(Ort::Session *session, ModelInit &mod, QWidget *parent)
    : QWidget(parent), session_(session), mod_(mod), frame_count_(0), is_paused_(false),
    video_path_("./target_video/ship_video.mp4") {

    // 创建简单布局，只包含图像标签
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 创建标签用于显示图像
    image_label_ = new QLabel();
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setMinimumSize(600, 600);
    layout->addWidget(image_label_);

    // 打开视频
    cap_.open(video_path_);
    if (!cap_.isOpened()) {
        utils::utf2ansi_out << "无法打开视频文件: " << video_path_ << '\n';
        return;
    }

    video_fps_ = cap_.get(cv::CAP_PROP_FPS);
    utils::utf2ansi_out << "视频输入帧率: " << video_fps_ << " FPS" << '\n';

    // 创建定时器用于更新帧
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MediaPlayer::update_frame);

    // 初始化计时点
    overall_start_ = std::chrono::high_resolution_clock::now();
    last_fps_update_time_ = overall_start_;

    // 启动定时器
    timer_->start(1000 / video_fps_);
}

MediaPlayer::~MediaPlayer() {
    timer_->stop();
    cap_.release();

    auto overallEnd = std::chrono::high_resolution_clock::now();
    double totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(overallEnd - overall_start_).count();
    double averageFps = (frame_count_ > 0) ? (frame_count_ * 1000.0 / totalTimeMs) : 0;

    utils::utf2ansi_out << "处理了 " << frame_count_ << " 帧, 总耗时："
        << totalTimeMs << " ms, 平均帧率：" << averageFps << " FPS" << '\n';
}

void MediaPlayer::update_frame() {
    if (is_paused_) {
        return;
    }

    cv::Mat frame;
    if (cap_.read(frame)) {
        // 处理推理和绘制边界框
        std::vector<DetectionResult> detections;
        {
            ImageInference image_inference(frame, session_, mod_);
            image_inference.draw_bounding_box();
            detections = image_inference.get_curr_info();
        }

        // 发送检测结果信号
        for (const auto &detection : detections) {
            // 计算边界框中心点
            QPoint center(
                detection.bbox.x + detection.bbox.width / 2,
                detection.bbox.y + detection.bbox.height / 2
            );

            // 转换置信度为百分比整数
            int confidence_percent = static_cast<int>(detection.confidence * 100);

            // 发送信号
            emit ship_detected(
                QString::fromStdString(detection.class_name),
                confidence_percent,
                center
            );
        }

        // 将 OpenCV Mat 转换为 QImage 并显示
        QImage qImage = mat_to_qimage(frame);

        // 设置图像
        image_label_->setPixmap(QPixmap::fromImage(qImage).scaled(
            image_label_->size(), Qt::KeepAspectRatio, Qt::FastTransformation));

        ++frame_count_;
        emit frame_processed(frame_count_);

        // 每秒更新一次FPS显示
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_update_time_).count();
        if (elapsed > 1000) {
            double currentFps = get_current_fps();
            emit fps_updated(currentFps);
            last_fps_update_time_ = now;

            // 定期清理缓存
            QPixmapCache::clear();
        }

        // 处理其他事件，保持UI响应
        QCoreApplication::processEvents();
    } else {
        timer_->stop();
        emit video_ended();
    }
}


double MediaPlayer::get_current_fps() const {
    auto now = std::chrono::high_resolution_clock::now();
    double totalTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - overall_start_).count();
    return (frame_count_ > 0) ? (frame_count_ * 1000.0 / totalTimeMs) : 0;
}

void MediaPlayer::set_video_path(const std::string &path) {
    video_path_ = path;
    reset_video();
}

void MediaPlayer::reset_video() {
    // 停止定时器
    timer_->stop();

    // 关闭并重新打开视频
    cap_.release();
    cap_.open(video_path_);

    if (!cap_.isOpened()) {
        utils::utf2ansi_out << "无法重新打开视频文件: " << video_path_ << '\n';
        return;
    }

    // 重置计数器和计时
    frame_count_ = 0;
    overall_start_ = std::chrono::high_resolution_clock::now();
    last_fps_update_time_ = overall_start_;

    emit frame_processed(0);
    emit fps_updated(0.0);

    // 如果不是暂停状态，重新启动定时器
    if (!is_paused_) {
        timer_->start(1000 / video_fps_);
    }
}

void MediaPlayer::play_pause(bool play) {
    is_paused_ = !play;
    if (!is_paused_ && !timer_->isActive()) {
        timer_->start(1000 / video_fps_);
    }
}

QImage MediaPlayer::mat_to_qimage(const cv::Mat &mat) {
    // 优化的转换方法
    if (mat.empty() || mat.depth() != CV_8U) {
        return {};
    }

    // BGR 格式转换为 RGB
    QImage qImage;
    if (mat.channels() == 3) {
        // 直接使用数据，避免额外的内存复制
        qImage = QImage(mat.data, mat.cols, mat.rows,
            static_cast<int>(mat.step), QImage::Format_RGB888);

        // 使用交换通道的方式替代cvtColor
        qImage = qImage.rgbSwapped();
    } else if (mat.channels() == 1) {
        qImage = QImage(mat.data, mat.cols, mat.rows,
            static_cast<int>(mat.step), QImage::Format_Grayscale8);
    } else {
        return {}; // 不支持的格式
    }
    return qImage.copy(); // 仍然需要复制以确保安全
}

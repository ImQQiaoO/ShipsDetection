#pragma once

#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <opencv2/opencv.hpp>

class VideoWindow : public QWidget {
    Q_OBJECT

public:
    VideoWindow(QWidget *parent = nullptr) : QWidget(parent) {
        // 设置窗口大小
        setWindowTitle("视频显示");
        resize(800, 600); // 设置窗口大小，可以根据需要调整

        // 创建一个QLabel来显示视频帧
        videoLabel = new QLabel(this);
        videoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        videoLabel->setGeometry(10, 10, 640, 480); // 设置QLabel在左上角显示

        // 视频路径
        video_path = "./target_video/ship_video.mp4";
        cap.open(video_path);

        if (!cap.isOpened()) {
            std::cerr << "无法打开视频文件!" << '\n';
            return;
        }

        // 获取视频的帧率
        input_fps = cap.get(cv::CAP_PROP_FPS);
        std::cout << "视频输入帧率: " << input_fps << " FPS" << std::endl;

        // 定时器，用于定期读取视频帧并更新显示
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &VideoWindow::updateFrame);
        timer->start(1000 / input_fps); // 根据视频帧率设置定时器间隔
    }

    ~VideoWindow() override {
        if (cap.isOpened()) {
            cap.release();
        }
    }

public slots:
    void updateFrame() {
        cv::Mat frame;
        if (cap.read(frame)) {
            // 将视频帧转换为QImage
            QImage img(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
            videoLabel->setPixmap(QPixmap::fromImage(img));
        }
    }

private:
    QLabel *videoLabel;
    cv::VideoCapture cap;
    double input_fps;
    QTimer *timer;
    std::string video_path;
};


#pragma once
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include "src/inference/ModelInit.h"
#include <chrono>

class MediaPlayer : public QWidget {
    Q_OBJECT

public:
    MediaPlayer(Ort::Session *session, ModelInit &mod, QWidget *parent = nullptr);
    ~MediaPlayer() override;

    QLabel *get_display_label() const { return image_label_; }
    int get_frame_count() const { return frame_count_; }
    double get_current_fps() const;
    void set_video_path(const std::string &path);
    void reset_video();
    cv::VideoCapture &get_cap() { return cap_; }

public slots:
    void update_frame();
    void play_pause(bool play);

signals:
    void frame_processed(int count);
    void fps_updated(double fps);
    void video_ended();

private:
    QLabel *image_label_;
    QTimer *timer_;
    cv::VideoCapture cap_;
    Ort::Session *session_;
    ModelInit &mod_;
    int frame_count_;
    std::string video_path_;
    double video_fps_;
    bool is_paused_;
    std::chrono::time_point<std::chrono::high_resolution_clock> overall_start_;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_fps_update_time_;

    QImage mat_to_qimage(const cv::Mat &mat);
};

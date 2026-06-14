#pragma once
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include "src/inference/ModelInit.h"
#include <chrono>
#include <memory>

#include "src/inference/ImageInference.h"
#include "src/inference/InferenceEngine.h"

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
    cv::Mat get_current_frame() const;
    static std::vector<DetectionResult> get_detections() { return detections_; }

public slots:
    void update_frame();
    void play_pause(bool play);

signals:
    void frame_processed(int count);
    void fps_updated(double fps);
    void video_ended();
    void ship_detected(const QString &ship_type, int confidence, const QPoint &position);

private:
    QLabel *image_label_;
    QTimer *timer_;
    cv::VideoCapture cap_;
    Ort::Session *session_;
    ModelInit &mod_;
    std::unique_ptr<InferenceEngine> engine_;
    int frame_count_;
    std::string video_path_;
    double video_fps_;
    bool is_paused_;
    cv::Mat current_frame_;  // 添加用于存储当前帧的成员变量
    std::chrono::time_point<std::chrono::high_resolution_clock> overall_start_;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_fps_update_time_;
    static std::vector<DetectionResult> detections_;

    static QImage mat_to_qimage(const cv::Mat &mat);
};

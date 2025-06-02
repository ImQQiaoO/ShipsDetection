#pragma once
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QTime>

#include "src/inference/ImageInference.h"

class MediaInfo : public QWidget {
    Q_OBJECT

public:
    explicit MediaInfo(QWidget *parent = nullptr);
    ~MediaInfo() override;

    // 设置视频信息
    void set_video_path(const QString &path) const;
    void set_video_resolution(int width, int height) const;
    void set_video_fps(double fps) const;
    void set_frame_count(int count) const;
    void update_current_fps(double fps) const;
    void update_processed_frames(int count) const;

signals:
    void play_pause_clicked(bool play);
    void reset_clicked();
    void open_file_clicked(); // 打开文件按钮被点击
    void capture_frame_clicked(std::vector<DetectionResult> results);

private slots:
    void on_play_pause_clicked();
    void on_open_file_clicked(); // 处理打开文件按钮点击
    void update_elapsed_time();
    void on_capture_frame_clicked(const std::vector<DetectionResult> &results);

private:
    // UI 组件
    QLabel *title_label_;
    QLabel *path_label_;
    QLabel *resolution_label_;
    QLabel *fps_label_;
    QLabel *frame_count_label_;
    QLabel *current_fps_label_;
    QLabel *processed_frames_label_;
    QLabel *elapsed_time_label_;
    QPushButton *play_pause_button_;
    QPushButton *reset_button_;
    QPushButton *open_file_button_; // 文件选择按钮
    QPushButton *capture_button_; // 捕获帧按钮
    QPushButton *history_button_; // 捕获帧按钮

    // 布局
    QVBoxLayout *main_layout_;
    QGridLayout *info_layout_;

    // 属性
    bool is_playing_;
    QTimer *elapsed_timer_;
    QTime elapsed_time_;

    // 初始化 UI
    void setup_ui();
};

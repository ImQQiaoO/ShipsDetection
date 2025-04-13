#pragma once
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QTime>

class MediaInfo : public QWidget {
    Q_OBJECT

public:
    explicit MediaInfo(QWidget *parent = nullptr);
    ~MediaInfo() override;

    // 设置视频信息
    void set_video_path(const QString &path) const;
    void set_video_resolution(int width, int height);
    void set_video_fps(double fps);
    void set_frame_count(int count);
    void update_current_fps(double fps);
    void update_processed_frames(int count);

signals:
    void play_pause_clicked(bool play);
    void reset_clicked();

private slots:
    void on_play_pause_clicked();
    void update_elapsed_time();

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

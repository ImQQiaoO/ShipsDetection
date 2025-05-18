#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QFileDialog>
#include "MediaPlayer.h"
#include "MediaInfo.h"
#include "LogPanel.h" // 添加 LogPanel 头文件
#include "src/inference/ModelInit.h"

class MainPanel : public QMainWindow {
    Q_OBJECT

public:
    MainPanel(Ort::Session *session, ModelInit &mod, QWidget *parent = nullptr);
    ~MainPanel() override = default;

private slots:
    void on_frame_processed(int count) const;
    void on_fps_updated(double fps) const;
    void on_video_ended();
    void on_play_pause_clicked(bool play) const;
    void on_reset_clicked() const;
    void on_open_file_clicked();

    // 处理检测到船舶的信号
    void on_ship_detected(const QString &ship_type, int confidence, const QPoint &position) const;
    void on_capture_frame() const;

private:
    MediaPlayer *media_player_;
    MediaInfo *media_info_;
    LogPanel *log_panel_; // 新增：日志面板
    QSplitter *splitter_;
    void setup_ui();
};

#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QFileDialog>
#include "MediaPlayer.h"
#include "MediaInfo.h"
#include "LogPanel.h"
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
    void on_ship_detected(const QString &ship_type, int confidence, const QPoint &position) const;
    void on_capture_frame(const std::vector<DetectionResult> &results) const;

private:
    MediaPlayer *media_player_;
    MediaInfo *media_info_;
    LogPanel *log_panel_;
    QSplitter *splitter_;
    void setup_ui();
};

#pragma once
#include <QMainWindow>
#include <QSplitter>
#include "MediaPlayer.h"
#include "MediaInfo.h"
#include "src/inference/ModelInit.h"

class MainPanel : public QMainWindow {
    Q_OBJECT

public:
    MainPanel(Ort::Session *session, ModelInit &mod, QWidget *parent = nullptr);
    ~MainPanel() override;

private slots:
    void on_frame_processed(int count);
    void on_fps_updated(double fps);
    void on_video_ended();
    void on_play_pause_clicked(bool play);
    void on_reset_clicked();

private:
    MediaPlayer *media_player_;
    MediaInfo *media_info_;
    QSplitter *splitter_;
    void setup_ui();
};

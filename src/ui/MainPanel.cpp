#include "MainPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

MainPanel::MainPanel(Ort::Session *session, ModelInit &mod, QWidget *parent)
    : QMainWindow(parent) {

    // 设置窗口
    setWindowTitle("视频检测结果");
    resize(1200, 800);

    // 创建视频播放器和信息面板
    media_player_ = new MediaPlayer(session, mod, this);
    media_info_ = new MediaInfo(this);

    // 设置UI布局
    setup_ui();

    // 连接信号和槽
    connect(media_player_, &MediaPlayer::frame_processed, this, &MainPanel::on_frame_processed);
    connect(media_player_, &MediaPlayer::fps_updated, this, &MainPanel::on_fps_updated);
    connect(media_player_, &MediaPlayer::video_ended, this, &MainPanel::on_video_ended);

    connect(media_info_, &MediaInfo::play_pause_clicked, this, &MainPanel::on_play_pause_clicked);
    connect(media_info_, &MediaInfo::reset_clicked, this, &MainPanel::on_reset_clicked);

    // 设置视频信息
    cv::VideoCapture &cap = media_player_->get_cap();
    if (cap.isOpened()) {
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        double fps = cap.get(cv::CAP_PROP_FPS);
        int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

        media_info_->set_video_path(QString::fromStdString("./target_video/ship_video.mp4"));
        media_info_->set_video_resolution(width, height);
        media_info_->set_video_fps(fps);
        media_info_->set_frame_count(totalFrames);
    }
}

MainPanel::~MainPanel() {
    // QMainWindow 会自动删除子部件
}

void MainPanel::setup_ui() {
    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 创建分割器
    splitter_ = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter_);

    // 将视频播放器和信息面板添加到分割器
    splitter_->addWidget(media_player_);
    splitter_->addWidget(media_info_);

    // 设置初始分割比例为2:1 (左:右)
    splitter_->setSizes(QList<int>() << 800 << 400);

    setCentralWidget(centralWidget);
}

void MainPanel::on_frame_processed(int count) const {
    media_info_->update_processed_frames(count);
}

void MainPanel::on_fps_updated(double fps) const {
    media_info_->update_current_fps(fps);
}

void MainPanel::on_video_ended() {
    // 可以添加视频结束时的处理逻辑
}

void MainPanel::on_play_pause_clicked(bool play) const {
    media_player_->play_pause(play);
}

void MainPanel::on_reset_clicked() const {
    media_player_->reset_video();
}


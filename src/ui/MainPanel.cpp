#include "MainPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>

MainPanel::MainPanel(Ort::Session *session, ModelInit &mod, QWidget *parent)
    : QMainWindow(parent) {

    // 设置窗口
    setWindowTitle("视频检测结果");
    resize(1200, 800);

    // 创建视频播放器和信息面板
    media_player_ = new MediaPlayer(session, mod, this);
    media_info_ = new MediaInfo(this);
    log_panel_ = new LogPanel(this); // 创建日志面板

    // 设置UI布局
    setup_ui();

    // 连接信号和槽
    connect(media_player_, &MediaPlayer::frame_processed, this, &MainPanel::on_frame_processed);
    connect(media_player_, &MediaPlayer::fps_updated, this, &MainPanel::on_fps_updated);
    connect(media_player_, &MediaPlayer::video_ended, this, &MainPanel::on_video_ended);

    // 假设 MediaPlayer 类有一个 ship_detected 信号
    // 如果没有，需要在 MediaPlayer 中添加这个信号
    // connect(media_player_, &MediaPlayer::ship_detected, this, &MainPanel::on_ship_detected);

    connect(media_info_, &MediaInfo::play_pause_clicked, this, &MainPanel::on_play_pause_clicked);
    connect(media_info_, &MediaInfo::reset_clicked, this, &MainPanel::on_reset_clicked);
    connect(media_info_, &MediaInfo::open_file_clicked, this, &MainPanel::on_open_file_clicked);

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

    // 添加一些测试日志（可选）
    // log_panel_->add_ship_log("货船", 95, QPoint(320, 240));
    // log_panel_->add_ship_log("渔船", 85, QPoint(450, 300));
    // log_panel_->add_ship_log("未知船只", 65, QPoint(200, 180));
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

    // 创建右侧面板容器
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    // 将信息面板和日志面板添加到右侧布局
    rightLayout->addWidget(media_info_);
    rightLayout->addWidget(log_panel_);

    // 设置右侧面板的大小策略
    media_info_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    log_panel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // 设置垂直布局的拉伸因子
    rightLayout->addWidget(media_info_, 2);
    rightLayout->addWidget(log_panel_, 4);

    // 将视频播放器和右侧面板添加到分割器
    splitter_->addWidget(media_player_);
    splitter_->addWidget(rightPanel);

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
    QMessageBox::information(this, "视频播放", "视频播放已结束");
}

void MainPanel::on_play_pause_clicked(bool play) const {
    media_player_->play_pause(play);
}

void MainPanel::on_reset_clicked() const {
    media_player_->reset_video();
}

void MainPanel::on_open_file_clicked() {
    // 打开文件选择对话框
    QString file_path = QFileDialog::getOpenFileName(
        this,
        "选择视频文件",
        QDir::homePath(),  // 从用户主目录开始
        "视频文件 (*.mp4 *.avi *.mkv *.mov);;所有文件 (*.*)"
    );

    // 如果用户选择了文件
    if (!file_path.isEmpty()) {
        // 更新媒体播放器的视频路径
        media_player_->set_video_path(file_path.toStdString());

        // 更新信息面板中的文件路径
        media_info_->set_video_path(file_path);

        // 重置视频播放
        media_player_->reset_video();
    }
}

void MainPanel::on_ship_detected(const QString &ship_type, int confidence, const QPoint &position) {
    // 将检测到的船舶信息添加到日志面板
    log_panel_->add_ship_log(ship_type, confidence, position);
}
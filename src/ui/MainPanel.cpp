#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "MainPanel.h"
#include "src/utils/Locale.hpp"
#include "src/ui/SnapShotPanel.h"

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

    // MediaPlayer 类有一个 ship_detected 信号
    connect(media_player_, &MediaPlayer::ship_detected, this, &MainPanel::on_ship_detected);

    connect(media_info_, &MediaInfo::play_pause_clicked, this, &MainPanel::on_play_pause_clicked);
    connect(media_info_, &MediaInfo::reset_clicked, this, &MainPanel::on_reset_clicked);
    connect(media_info_, &MediaInfo::open_file_clicked, this, &MainPanel::on_open_file_clicked);

    connect(media_info_, &MediaInfo::capture_frame_clicked, this, &MainPanel::on_capture_frame);
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
        "D:\\Playground\\test\\ShipsDetection\\target_video",
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

void MainPanel::on_ship_detected(const QString &ship_type, int confidence, const QPoint &position) const {
    // 将检测到的船舶信息添加到日志面板
    log_panel_->add_ship_log(ship_type, confidence, position);
}

void MainPanel::on_capture_frame(const std::vector<DetectionResult> &results) const {
    // 获取当前帧并保存为图像
    cv::Mat current_frame = media_player_->get_current_frame();
    //std::cout << ship_type.toStdString() << " " << confidence << " " << position.x() << " " << position.y() << std::endl;

    if (!current_frame.empty()) {
        // 生成带有时间戳的文件名
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");

        QDir saveDir(save_path);  // Snapshot 保存目录
        if (!saveDir.exists()) {
            bool saved = saveDir.mkpath(".");
            if (!saved) {
                log_panel_->add_log("错误：无法创建保存目录。请检查权限或路径是否正确。");
                return;
            }
        }
        QString filename = saveDir.filePath("capture_" + timestamp + ".png");
        cv::imwrite(filename.toStdString(), current_frame);

        // 在日志面板中记录拍照信息
        const QString log_message = "已截取当前帧并保存为: " + filename;
        log_panel_->add_log(log_message);

        SnapShotPanel *snapshot_dialog = new SnapShotPanel(current_frame, results, filename, log_panel_, const_cast<MainPanel *>(this));
        snapshot_dialog->setAttribute(Qt::WA_DeleteOnClose);
        snapshot_dialog->show();
    } else {
        log_panel_->add_log("无法截取当前帧");
    }
}

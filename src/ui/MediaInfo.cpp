#include "MediaInfo.h"
#include <QFileInfo>
#include <QStyle>

#include "MediaPlayer.h"
#include "HistoryPanel.h"

MediaInfo::MediaInfo(QWidget *parent)
    : QWidget(parent), is_playing_(true), elapsed_time_(0, 0, 0) {
    setup_ui();

    // 启动计时器，每秒更新一次时间
    elapsed_timer_ = new QTimer(this);
    connect(elapsed_timer_, &QTimer::timeout, this, &MediaInfo::update_elapsed_time);
    elapsed_timer_->start(1000);
}

MediaInfo::~MediaInfo() {
    elapsed_timer_->stop();
    delete elapsed_timer_;
}

void MediaInfo::setup_ui() {
    // 创建标题
    title_label_ = new QLabel("媒体信息面板", this);
    QFont titleFont = title_label_->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title_label_->setFont(titleFont);

    // 创建信息标签
    path_label_ = new QLabel("路径: 未指定", this);
    resolution_label_ = new QLabel("分辨率: 未知", this);
    fps_label_ = new QLabel("帧率: 未知", this);
    frame_count_label_ = new QLabel("总帧数: 未知", this);
    current_fps_label_ = new QLabel("当前FPS: 0.0", this);
    processed_frames_label_ = new QLabel("已处理帧数: 0", this);
    elapsed_time_label_ = new QLabel("运行时间: 00:00:00", this);

    // 创建按钮
    play_pause_button_ = new QPushButton("暂停", this);
    reset_button_ = new QPushButton("重置", this);
    open_file_button_ = new QPushButton("选择视频文件", this);
    open_file_button_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton)); // 添加文件图标
    open_file_button_->setMinimumWidth(380); // 设置最小宽度
    open_file_button_->setMinimumHeight(35); // 设置最小高度

    // 创建拍照按钮
    capture_button_ = new QPushButton("拍照", this);
    capture_button_->setMinimumWidth(380); // 设置最小宽度
    capture_button_->setMinimumHeight(35); // 设置最小高度
    connect(capture_button_, &QPushButton::clicked, this, [this]() {
        auto condition = MediaPlayer::get_detections();
        on_capture_frame_clicked(condition);
    });

    // 创建历史按钮
    history_button_ = new QPushButton("历史", this);
    history_button_->setMinimumWidth(380); // 设置最小宽度
    history_button_->setMinimumHeight(35); // 设置最小高度
    connect(history_button_, &QPushButton::clicked, this, []() {
        // 弹出历史面板
        auto *panel = new HistoryPanel();
        panel->setAttribute(Qt::WA_DeleteOnClose);
        panel->setWindowModality(Qt::ApplicationModal);
        panel->show();
    });

    // 连接按钮信号
    connect(play_pause_button_, &QPushButton::clicked, this, &MediaInfo::on_play_pause_clicked);
    connect(reset_button_, &QPushButton::clicked, this, &MediaInfo::reset_clicked);
    connect(open_file_button_, &QPushButton::clicked, this, &MediaInfo::on_open_file_clicked);

    // 创建信息网格布局
    info_layout_ = new QGridLayout();
    info_layout_->addWidget(new QLabel("媒体信息:", this), 0, 0, 1, 2);
    info_layout_->addWidget(path_label_, 1, 0, 1, 2);
    info_layout_->addWidget(resolution_label_, 2, 0);
    info_layout_->addWidget(fps_label_, 2, 1);
    info_layout_->addWidget(frame_count_label_, 3, 0);
    info_layout_->addWidget(elapsed_time_label_, 3, 1);

    info_layout_->addWidget(new QLabel("运行状态:", this), 4, 0, 1, 2);
    info_layout_->addWidget(current_fps_label_, 5, 0);
    info_layout_->addWidget(processed_frames_label_, 5, 1);

    // 创建文件按钮布局
    QHBoxLayout *file_layout = new QHBoxLayout();
    file_layout->addWidget(open_file_button_);

    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addWidget(capture_button_);  // 添加拍照按钮到布局

    QHBoxLayout *button1_layout = new QHBoxLayout;
    button1_layout->addWidget(history_button_);

    // 创建按钮布局
    QHBoxLayout *control_layout = new QHBoxLayout();
    control_layout->addWidget(play_pause_button_);
    control_layout->addWidget(reset_button_);

    // 创建主布局
    main_layout_ = new QVBoxLayout(this);
    main_layout_->addWidget(title_label_, 0, Qt::AlignCenter);
    main_layout_->addLayout(info_layout_);

    // 添加一个小间距
    main_layout_->addSpacing(10);

    // 添加按钮布局，紧跟在信息部分之后
    main_layout_->addLayout(file_layout);
    main_layout_->addLayout(button_layout);
    main_layout_->addLayout(button1_layout);
    main_layout_->addLayout(control_layout);

    // 添加弹性空间，将所有内容推向顶部
    main_layout_->addStretch(1);

    // 设置样式
    setStyleSheet(
        "QLabel { font-size: 12px; }"
        "QPushButton { font-size: 13px; padding: 6px 12px; }"
    );
}

void MediaInfo::set_video_path(const QString &path) const {
    QFileInfo fileInfo(path);
    path_label_->setText("路径: " + fileInfo.fileName());
    path_label_->setToolTip(path);
}

void MediaInfo::set_video_resolution(int width, int height) const {
    resolution_label_->setText(QString("分辨率: %1 x %2").arg(width).arg(height));
}

void MediaInfo::set_video_fps(double fps) const {
    fps_label_->setText(QString("帧率: %1").arg(fps, 0, 'f', 1));
}

void MediaInfo::set_frame_count(int count) const {
    if (count > 0) {
        frame_count_label_->setText(QString("总帧数: %1").arg(count));
    } else {
        frame_count_label_->setText("总帧数: 未知");
    }
}

void MediaInfo::update_current_fps(double fps) const {
    current_fps_label_->setText(QString("当前FPS: %1").arg(fps, 0, 'f', 1));
}

void MediaInfo::update_processed_frames(int count) const {
    processed_frames_label_->setText(QString("已处理帧数: %1").arg(count));
}

void MediaInfo::on_play_pause_clicked() {
    is_playing_ = !is_playing_;
    if (is_playing_) {
        play_pause_button_->setText("暂停");
        elapsed_timer_->start();
    } else {
        play_pause_button_->setText("播放");
        elapsed_timer_->stop();
    }
    emit play_pause_clicked(is_playing_);
}

void MediaInfo::on_open_file_clicked() {
    // 发出打开文件信号，由MainPanel处理
    emit open_file_clicked();
}

void MediaInfo::on_capture_frame_clicked(const std::vector<DetectionResult> &results) {
    emit capture_frame_clicked(results);
}

void MediaInfo::update_elapsed_time() {
    elapsed_time_ = elapsed_time_.addSecs(1);
    elapsed_time_label_->setText(QString("运行时间: %1").arg(elapsed_time_.toString("hh:mm:ss")));
}

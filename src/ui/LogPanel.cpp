#include "LogPanel.h"
#include <QScrollBar>
#include <QFont>

LogPanel::LogPanel(QWidget *parent) : QWidget(parent) {
    setup_ui();
}

void LogPanel::setup_ui() {
    // 创建标题
    title_label_ = new QLabel("检测日志", this);
    QFont titleFont = title_label_->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title_label_->setFont(titleFont);

    //// 固定高度
    //setMinimumHeight(500);  // 设置最小高度
    //setMaximumHeight(500);  // 设置最大高度

    // 创建日志文本框
    log_text_ = new QTextEdit(this);
    log_text_->setReadOnly(true); // 设置为只读
    log_text_->setLineWrapMode(QTextEdit::WidgetWidth); // 自动换行

    // 设置文本框样式
    log_text_->setStyleSheet(
        "QTextEdit {"
        "  background-color: #f5f5f5;"
        "  border: 1px solid #ddd;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  font-family: Consolas, Monospace;"
        "  font-size: 12px;"
        "}"
    );

    // 创建清空按钮
    clear_button_ = new QPushButton("清空日志", this);
    clear_button_->setMinimumHeight(30);
    connect(clear_button_, &QPushButton::clicked, this, &LogPanel::clear_logs);

    // 创建主布局
    main_layout_ = new QVBoxLayout(this);
    main_layout_->addWidget(title_label_, 0, Qt::AlignCenter);
    main_layout_->addWidget(log_text_);
    main_layout_->addWidget(clear_button_);

    // 添加初始提示信息
    log_text_->append("<i>日志将显示在此处...</i>");
}

void LogPanel::add_ship_log(const QString &ship_type, int confidence, const QPoint &position) {
    // 格式化并添加日志条目
    QString log_entry = format_log_entry(ship_type, confidence, position);
    log_text_->append(log_entry);

    // 滚动到底部以显示最新日志
    QScrollBar *scrollbar = log_text_->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void LogPanel::clear_logs() const {
    log_text_->clear();
    log_text_->append("<i>日志已清空</i>");
}

QString LogPanel::format_log_entry(const QString &ship_type, int confidence, const QPoint &position) {
    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 根据置信度设置不同的颜色
    QString confidence_color;
    if (confidence >= 90) {
        confidence_color = "green";
    } else if (confidence >= 70) {
        confidence_color = "orange";
    } else {
        confidence_color = "red";
    }

    // 格式化日志条目
    return QString("<b>[%1]</b> 检测到<span style='color:blue;'>%2</span>，"
        "置信度：<span style='color:%3;'>%4%</span>，"
        "位置：(%5, %6)")
        .arg(timestamp)
        .arg(ship_type)
        .arg(confidence_color)
        .arg(confidence)
        .arg(position.x())
        .arg(position.y());
}
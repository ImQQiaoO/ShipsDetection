#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>

class LogPanel : public QWidget {
    Q_OBJECT

public:
    explicit LogPanel(QWidget *parent = nullptr);
    ~LogPanel() override = default;

    // 添加船舶检测日志
    void add_ship_log(const QString &ship_type, int confidence, const QPoint &position);

    // 清空日志
    void clear_logs();

private:
    // UI 组件
    QLabel *title_label_;
    QTextEdit *log_text_;
    QPushButton *clear_button_;

    // 布局
    QVBoxLayout *main_layout_;

    // 初始化 UI
    void setup_ui();

    // 格式化日志条目
    QString format_log_entry(const QString &ship_type, int confidence, const QPoint &position);
};

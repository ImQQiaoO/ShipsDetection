#pragma once

#include <QDialog>
#include <QtCharts>
#include <QPieSeries>
#include <QChartView>

class HistoryPanel : public QDialog {
    Q_OBJECT

    QPieSeries *series_;
    QTableWidget *table_widget_;
public:
    HistoryPanel();
private slots:
    void onDeleteRow(); // 删除行槽函数
private:
    void setupTableContextMenu(); // 设置表格右键菜单
    int context_row_ = -1; // 记录右键点击的行
};

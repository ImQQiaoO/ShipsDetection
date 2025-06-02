#pragma once

#include <QDialog>
#include <QtCharts>
#include <QPieSeries>
#include <QChartView>

#include "src/utils/JsonRepository.hpp"

class HistoryPanel : public QDialog {
    Q_OBJECT

    QPieSeries *series_;
    QTableWidget *table_widget_;
public:
    HistoryPanel();
private slots:
    void onDeleteRow(); // 删除行槽函数
    void onCellEntered(int row, int column); // 悬停槽
private:
    void setupTableContextMenu(); // 设置表格右键菜单
    int context_row_ = -1; // 记录右键点击的行

    QPointer<QLabel> preview_label_; // 图片预览
    std::vector<SnapShotItem> saved_items_; // 保存快照数据
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

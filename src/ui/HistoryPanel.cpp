#include "HistoryPanel.h"
#include "src/utils/JsonRepository.hpp"
#include <QTableWidget>
#include <map>

HistoryPanel::HistoryPanel() {
    setWindowTitle(tr("历史记录")); // tr() 以便翻译
    resize(600, 800);
    setMinimumSize(400, 600);  // 最小尺寸
    setMaximumSize(800, 1200); // 最大尺寸
    std::vector<SnapShotItem> saved_items = JsonRepository::read_from_json("./snapshots.json");

    std::map<std::string, size_t> inventory;
    for (auto& saved_item : saved_items) {
        ++inventory[saved_item.ship_type];
    }

    // ———— 上方：统计图展示 ————
    series_ = new QPieSeries();
    for (const auto &item : inventory) {
        series_->append(QString::fromStdString(item.first), item.second);
    }

    // 设置内径比例创建环形图
    series_->setHoleSize(0.35);  // 内径占外径的35%

    QChart *chart = new QChart();
    chart->addSeries(series_);
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);


    // ———— 下方：四列表格 ————
    // QTableWidget 自带滚动条，无需外部 QScrollArea
    table_widget_ = new QTableWidget(this);
    table_widget_->setColumnCount(4);
    table_widget_->setHorizontalHeaderLabels({
        tr("序号"),
        tr("类型"),
        tr("置信度"),
        tr("船舶编号")
        });
    // 设置初始行数
    table_widget_->setRowCount(saved_items.size());

    // 列宽均匀分布，可根据需要调整
    table_widget_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );
    table_widget_->verticalHeader()->setVisible(false);

    for (int i = 0; i < saved_items.size(); ++i) {
        const auto &result = saved_items[i];
        // 设置每个单元格的内容并居中对齐
        QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(i + 1));
        item1->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 0, item1);  // 序号
        QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromStdString(result.ship_type));
        item2->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 1, item2);  // 类型
        QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(result.confidence, 'f', 2));
        item3->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 2, item3);  // 置信度
        QTableWidgetItem *item4 = new QTableWidgetItem(QString::fromStdString(result.identifier)); // 船舶编号
        item4->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 3, item4);
    }

    // ———— 布局 ————
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(chartView);     // 上方图片
    mainLayout->addWidget(table_widget_);    // 下方表格
    setLayout(mainLayout);

    adjustSize();
}


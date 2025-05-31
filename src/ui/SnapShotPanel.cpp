#include "SnapShotPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QTableWidget>
#include <QHeaderView>

#include "src/inference/DrawBoundingBox.h"

SnapShotPanel::SnapShotPanel(const QImage &image, const std::vector<DetectionResult> &results, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("当前帧")); // tr() 以便翻译

    // ———— 上方：图片展示 ————
    imageLabel = new QLabel(this);
    QPixmap originalPixmap = QPixmap::fromImage(image);
    const int maxWidth = 800;
    const int maxHeight = 300;
    QPixmap scaledPixmap = originalPixmap.scaled(
        maxWidth, maxHeight,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMaximumHeight(maxHeight);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // ———— 下方：四列表格 ————
    // QTableWidget 自带滚动条，无需外部 QScrollArea
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(4);
    // 可根据需要修改列标题
    tableWidget->setHorizontalHeaderLabels({
        tr("序号"),
        tr("类型"),
        tr("置信度"),
        tr("船舶编号")
        });
    // 设置初始行数
    tableWidget->setRowCount(results.size());

    // 列宽均匀分布，可根据需要调整
    tableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );
    tableWidget->verticalHeader()->setVisible(false);

    for (int i = 0; i < results.size(); ++i) {
        const auto &result = results[i];
        // 设置每个单元格的内容并居中对齐
        QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(i + 1));
        item1->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 0, item1);  // 序号
        QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromStdString(DrawBoundingBox::class_names[static_cast<size_t>(std::stoul(result.class_name))]));
        item2->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 1, item2);  // 类型
        QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(result.confidence, 'f', 2));
        item3->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 2, item3);  // 置信度
        QTableWidgetItem *item4 = new QTableWidgetItem(QString::fromStdString(result.class_name)); // 船舶编号
        item4->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(i, 3, item4);
    }

    // ———— 布局 ————
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(imageLabel);     // 上方图片
    mainLayout->addWidget(tableWidget);    // 下方表格
    setLayout(mainLayout);

    adjustSize();
}

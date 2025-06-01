#include "SnapShotPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QTableWidget>
#include <QHeaderView>

#include "src/inference/DrawBoundingBox.h"
#include "src/utils/JsonRepository.hpp"

SnapShotPanel::SnapShotPanel(const QImage &image, const std::vector<DetectionResult> &results, const QString &filename, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("当前帧")); // tr() 以便翻译

    filename_ = filename;

    // ———— 上方：图片展示 ————
    image_label_ = new QLabel(this);
    QPixmap originalPixmap = QPixmap::fromImage(image);
    const int maxWidth = 800;
    const int maxHeight = 300;
    QPixmap scaledPixmap = originalPixmap.scaled(
        maxWidth, maxHeight,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    image_label_->setPixmap(scaledPixmap);
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setMaximumHeight(maxHeight);
    image_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // ———— 下方：四列表格 ————
    // QTableWidget 自带滚动条，无需外部 QScrollArea
    table_widget_ = new QTableWidget(this);
    table_widget_->setColumnCount(4);
    // 可根据需要修改列标题
    table_widget_->setHorizontalHeaderLabels({
        tr("序号"),
        tr("类型"),
        tr("置信度"),
        tr("船舶编号")
        });
    // 设置初始行数
    table_widget_->setRowCount(results.size());

    // 列宽均匀分布，可根据需要调整
    table_widget_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch
    );
    table_widget_->verticalHeader()->setVisible(false);

    for (int i = 0; i < results.size(); ++i) {
        const auto &result = results[i];
        // 设置每个单元格的内容并居中对齐
        QTableWidgetItem *item1 = new QTableWidgetItem(QString::number(i + 1));
        item1->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 0, item1);  // 序号
        QTableWidgetItem *item2 = new QTableWidgetItem(QString::fromStdString(DrawBoundingBox::class_names[static_cast<size_t>(std::stoul(result.class_name))]));
        item2->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 1, item2);  // 类型
        QTableWidgetItem *item3 = new QTableWidgetItem(QString::number(result.confidence, 'f', 2));
        item3->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 2, item3);  // 置信度
        QTableWidgetItem *item4 = new QTableWidgetItem(QString::fromStdString(result.class_name)); // 船舶编号
        item4->setTextAlignment(Qt::AlignCenter);
        table_widget_->setItem(i, 3, item4);
    }

    // ———— 布局 ————
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(image_label_);     // 上方图片
    mainLayout->addWidget(table_widget_);    // 下方表格
    setLayout(mainLayout);

    adjustSize();
}

QVector<QVector<QString>> SnapShotPanel::get_table_contents() const {
    QVector<QVector<QString>> contents;
    int rowCount = table_widget_->rowCount();
    int colCount = table_widget_->columnCount();
    contents.resize(rowCount);
    
    for (int i = 0; i < rowCount; ++i) {
        contents[i].resize(colCount);
        for (int j = 0; j < colCount; ++j) {
            QTableWidgetItem *item = table_widget_->item(i, j);
            if (item) {
                contents[i][j] = item->text();
            } else {
                contents[i][j] = QString(); // 如果没有内容则设置为空
            }
        }
    }
    return contents;
}

void SnapShotPanel::closeEvent(QCloseEvent *event) {
    auto contents = get_table_contents();

    // 打印表格内容到调试输出
    for (const auto &row : contents) {
        QStringList row_str;
        for (const auto &cell : row) {
            row_str << cell;
        }
        qDebug() << row_str.join(" | ");
    }

    JsonRepository jr(contents, filename_);
    jr.save_to_file("./snapshots.json");

    QDialog::closeEvent(event);
}

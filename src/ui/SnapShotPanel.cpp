#include "SnapShotPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QTableWidget>
#include <QHeaderView>

#include "LogPanel.h"
#include "src/inference/DrawBoundingBox.h"
#include "src/utils/JsonRepository.hpp"
#include "src/ocr/ShipNumOCR.h"

SnapShotPanel::SnapShotPanel(const cv::Mat &image, const std::vector<DetectionResult> &results, const QString &filename,
    const LogPanel *log_panel, QWidget *parent) : QDialog(parent), curr_frame_(image) {

    setWindowTitle(tr("当前帧")); // tr() 以便翻译

    filename_ = filename;
    std::vector<std::string> ship_identifiers = get_ocr_res(results);
    process_image(results);
    auto qimage = cv2qimage(image, log_panel);
    // ———— 上方：图片展示 ————
    image_label_ = new QLabel(this);
    QPixmap originalPixmap = QPixmap::fromImage(qimage);
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
        QTableWidgetItem *item4 = new QTableWidgetItem(QString::fromStdString(ship_identifiers[i])); // 船舶编号
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

std::vector<std::string> SnapShotPanel::get_ocr_res(const std::vector<DetectionResult> &results) const {
    std::vector<std::string> ship_identifiers(results.size());
    cv::Mat ocr_image = curr_frame_.clone();
    for (const auto &result : results) {
        cv::Mat roi_image = ocr_image(result.bbox);
        ShipNumOCR ship_ocr;
        ship_identifiers.push_back(ship_ocr.recognizeText(roi_image));
    }
    return ship_identifiers;
}

QImage SnapShotPanel::cv2qimage(const cv::Mat &frame, const LogPanel *log_panel) {
    QImage qImg;
    if (frame.channels() == 3) { // 彩色图像
        cv::Mat rgb_frame;
        cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
        qImg = QImage(rgb_frame.data, rgb_frame.cols, rgb_frame.rows, rgb_frame.step, QImage::Format_RGB888).copy();
    } else if (frame.channels() == 1) { // 灰度图像
        qImg = QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_Grayscale8).copy();
    } else {
        log_panel->add_log("错误：不支持的图像格式用于显示。");
        return {};
    }
    if (qImg.isNull()) {
        log_panel->add_log("错误：无法将OpenCV图像转换为QImage。");
        return {};
    }
    return qImg;
}

void SnapShotPanel::process_image(const std::vector<DetectionResult> &results) {
    // 先保存原始帧到ocr_image
    cv::Mat overlay = curr_frame_.clone();  // 复制一份用来画不透明矩形
    double alpha = 125.0 / 255.0;  // alpha 透明度
    for (size_t i = 0; i < results.size(); ++i) {
        const auto &bbox = results[i].bbox;
        auto rgb_arr = DrawBoundingBox::box_color[stoi(results[i].class_name)];
        cv::rectangle(overlay, bbox, cv::Scalar(rgb_arr[0], rgb_arr[1], rgb_arr[2]), cv::FILLED);
        std::string text = std::to_string(i + 1);
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        double fontScale = 5;
        int thickness = 2;
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
        int text_x = bbox.x + (bbox.width - textSize.width) / 2;
        int text_y = bbox.y + (bbox.height + textSize.height) / 2;
        cv::putText(overlay, text, cv::Point(text_x, text_y), fontFace, fontScale, cv::Scalar(255, 255, 255), thickness);
    }
    // 用 addWeighted 把 'overlay' 混合到 curr_frame_ 上
    cv::addWeighted(overlay, alpha, curr_frame_, 1.0 - alpha, 0, curr_frame_);
}

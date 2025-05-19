#include "SnapShotPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <opencv2/highgui.hpp>
#include <QScrollArea>

SnapShotPanel::SnapShotPanel(const QImage &image, QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("当前帧")); // tr() for potential translation

    // 左边的图像部分
    imageLabel = new QLabel(this);
    QPixmap originalPixmap = QPixmap::fromImage(image);
    imageLabel->setPixmap(QPixmap::fromImage(image));
    int targetWidth = 800;
    int targetHeight = 600;
    QPixmap scaledPixmap = originalPixmap.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->setAlignment(Qt::AlignCenter); // 居中显示图片

    // 右边的可滚动面板
    QWidget *scrollableWidget = new QWidget(this);
    QVBoxLayout *scrollableLayout = new QVBoxLayout(scrollableWidget);

    // 在右边添加一些内容示例
    QLabel *scrollableLabel = new QLabel("滚动区域内容", scrollableWidget);
    scrollableLayout->addWidget(scrollableLabel);
    scrollableWidget->setLayout(scrollableLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(scrollableWidget);
    scrollArea->setWidgetResizable(true); // 使内容可调整大小

    // 创建水平布局，将左右部分放置在一起
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(imageLabel); // 左边的图像
    mainLayout->addWidget(scrollArea); // 右边的可滚动面板

    setLayout(mainLayout);
    adjustSize();
}

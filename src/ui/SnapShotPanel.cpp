#include "SnapShotPanel.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <opencv2/highgui.hpp>
#include <QScrollArea>

SnapShotPanel::SnapShotPanel(const QImage &image, QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("当前帧")); // tr() for potential translation

    // 图片部分
    imageLabel = new QLabel(this);
    QPixmap originalPixmap = QPixmap::fromImage(image);
    int maxWidth = 800;
    int maxHeight = 300;
    QPixmap scaledPixmap = originalPixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);


    // 控制图片最大高度（比如300像素），让图片不会太大
    imageLabel->setMaximumHeight(300);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 右边的可滚动面板（现在在下方）
    QWidget *scrollableWidget = new QWidget(this);
    QVBoxLayout *scrollableLayout = new QVBoxLayout(scrollableWidget);
    QLabel *scrollableLabel = new QLabel("滚动区域内容", scrollableWidget);
    scrollableLayout->addWidget(scrollableLabel);
    scrollableWidget->setLayout(scrollableLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(scrollableWidget);
    scrollArea->setWidgetResizable(true);

    // 垂直布局：图片在上，滚动区在下
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(imageLabel);   // 上方图片
    mainLayout->addWidget(scrollArea);   // 下方滚动区

    setLayout(mainLayout);
    adjustSize();
}
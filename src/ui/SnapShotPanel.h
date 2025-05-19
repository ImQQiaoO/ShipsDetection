#pragma once
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

class SnapShotPanel : public QDialog {
    Q_OBJECT

public:
    explicit SnapShotPanel(const QImage &image, QWidget *parent = nullptr);
    ~SnapShotPanel() override = default;

private:
    QLabel *imageLabel;
};
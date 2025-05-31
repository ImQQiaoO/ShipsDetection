#pragma once
#include <QDialog>
#include "src/inference/ImageInference.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QVBoxLayout;
class QTableWidget;
QT_END_NAMESPACE

class SnapShotPanel : public QDialog {
    Q_OBJECT

public:
    explicit SnapShotPanel(const QImage &image, const std::vector<DetectionResult> &results, QWidget *parent = nullptr);
    ~SnapShotPanel() override = default;

private:
    QLabel *imageLabel;
    QTableWidget *tableWidget;
    QVector<QVector<QString>> get_table_contents() const;
    void closeEvent(QCloseEvent *event) override;
};
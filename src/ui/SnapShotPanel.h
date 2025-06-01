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
    explicit SnapShotPanel(const QImage &image, const std::vector<DetectionResult> &results, const QString &filename, QWidget *parent = nullptr);
    ~SnapShotPanel() override = default;

private:
    QLabel *image_label_;
    QTableWidget *table_widget_;
    QString filename_;
    QVector<QVector<QString>> get_table_contents() const;
    void closeEvent(QCloseEvent *event) override;
};
#pragma once
#include <QDialog>

#include "LogPanel.h"
#include "src/inference/ImageInference.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QVBoxLayout;
class QTableWidget;
QT_END_NAMESPACE

class SnapShotPanel : public QDialog {
    Q_OBJECT

public:
    explicit SnapShotPanel(const cv::Mat &image, const std::vector<DetectionResult> &results, const QString &filename,
        const LogPanel *log_panel, QWidget *parent = nullptr);
    ~SnapShotPanel() override = default;

private:
    cv::Mat curr_frame_;
    QLabel *image_label_;
    QTableWidget *table_widget_;
    QString filename_;
    QVector<QVector<QString>> get_table_contents() const;
    void closeEvent(QCloseEvent *event) override;
    //std::vector<std::string> get_ocr_res(const std::vector<DetectionResult>& results) const;
    static QImage cv2qimage(const cv::Mat &frame, const LogPanel *log_panel);
    void process_image(const std::vector<DetectionResult>& results);
};
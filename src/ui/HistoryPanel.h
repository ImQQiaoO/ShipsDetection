#pragma once

#include <QDialog>
#include <QtCharts>
#include <QPieSeries>
#include <QChartView>

class HistoryPanel : public QDialog {
    Q_OBJECT

    QPieSeries *series_;
    QTableWidget *table_widget_;
public:
    HistoryPanel();
};
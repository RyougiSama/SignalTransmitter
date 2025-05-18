#pragma once

#include <QChartView>
#include <QLineSeries>

class TimeViewEncoded  : public QChartView
{
    Q_OBJECT

public:
    TimeViewEncoded(QWidget *parent);
    ~TimeViewEncoded();

private:
    QLineSeries *disp_series_ = nullptr;
};

#include "timeviewencoded.h"
#include <QValueAxis>

TimeViewEncoded::TimeViewEncoded(QWidget *parent)
    : QChartView(parent)
    , disp_series_(new QLineSeries(this))
{
    // Font setting
    QFont font;
    font.setBold(true);
    // Create chart and add data series
    auto chart = new QChart();
    chart->setTitleFont(font);
    chart->setTitle("Encoded Time-Domain Wave");
    chart->legend()->close();
    chart->addSeries(disp_series_);
    // Create axes
    auto axisX = new QValueAxis();
    axisX->setTitleText("Time (s)");
    axisX->setRange(0, 5);
    axisX->setTickCount(5);
    auto axisY = new QValueAxis();
    axisY->setTitleText("Amplitude");
    axisY->setRange(-1, 1);
    axisY->setTickCount(9);
    // Add axes to chart
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    disp_series_->attachAxis(axisX);
    disp_series_->attachAxis(axisY);
    // Set chart to view
    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
}

TimeViewEncoded::~TimeViewEncoded()
{}

#pragma once

#include <QChartView>

class AudioWaveformView  : public QChartView
{
    Q_OBJECT

public:
    AudioWaveformView(QWidget *parent);
    ~AudioWaveformView();
};


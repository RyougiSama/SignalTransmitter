#pragma once

#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QAudioFormat>

class AudioWaveformView  : public QChartView
{
    Q_OBJECT

public:
    AudioWaveformView(QWidget *parent);
    ~AudioWaveformView();

    void UpdateWaveform(const QByteArray &audio_data, const QAudioFormat &format);
    void StartDisplay();
    void StopDisplay();
    void ClearDisplay();

public:
    static constexpr double kDisplayDuration{ 0.05 };    // 显示时长

private:
    void InitChart();
    void UpdateDisplay();
    QList<double> ConvertToDisplayData(const QByteArray &pcm_data, const QAudioFormat &format);

private:
    QLineSeries *waveform_series_;
    QValueAxis *axis_x_;
    QValueAxis *axis_y_;    // 显示参数
    QList<double> sample_buffer_;
    static constexpr int kMaxDisplayPoints{ 1000 };     // 最大显示点数
    // 状态控制
    QAudioFormat current_format_;
    bool is_displaying_{ false };
};

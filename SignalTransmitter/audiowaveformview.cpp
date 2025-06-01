#include "audiowaveformview.h"
#include <QChart>
#include <QtMath>

AudioWaveformView::AudioWaveformView(QWidget *parent)
    : QChartView(parent)
    , waveform_series_(new QLineSeries(this))
    , axis_x_(new QValueAxis(this))
    , axis_y_(new QValueAxis(this))
{
    InitChart();
}

AudioWaveformView::~AudioWaveformView()
{
}

void AudioWaveformView::InitChart()
{
    // 字体设置
    QFont font;
    font.setBold(true);
    // 创建图表
    auto chart = new QChart();
    chart->setTitleFont(font);
    chart->setTitle("Audio Waveform View");
    chart->legend()->hide();
    chart->addSeries(waveform_series_);
    // 设置波形样式
    QPen pen(Qt::blue);
    pen.setWidth(1);
    waveform_series_->setPen(pen);
    // 配置X轴（时间轴）
    axis_x_->setTitleText("Time (s)");
    axis_x_->setRange(0, kDisplayDuration);
    axis_x_->setTickCount(7);
      // 配置Y轴（振幅轴）
    axis_y_->setTitleText("Amplitude");
    axis_y_->setRange(-1.0, 1.0);
    axis_y_->setTickCount(5);
    axis_y_->setLabelFormat("%.1f");
    // 添加坐标轴到图表
    chart->addAxis(axis_x_, Qt::AlignBottom);
    chart->addAxis(axis_y_, Qt::AlignLeft);
    waveform_series_->attachAxis(axis_x_);
    waveform_series_->attachAxis(axis_y_);
    // 设置图表到视图
    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
}

void AudioWaveformView::UpdateWaveform(const QByteArray &audio_data, const QAudioFormat &format)
{
    if (!is_displaying_ || audio_data.isEmpty()) {
        return;
    }
    current_format_ = format;
    // 转换音频数据为显示数据
    const auto new_samples = ConvertToDisplayData(audio_data, format);
    if (new_samples.isEmpty()) {
        return;
    }
    // 添加新样本到缓冲区
    for (const auto &sample : new_samples) {
        sample_buffer_.append(sample);
    }
    // 保持固定的显示点数，移除多余的旧数据
    while (sample_buffer_.size() > kMaxDisplayPoints) {
        sample_buffer_.removeFirst();
    }
    // 更新显示
    UpdateDisplay();
}

QList<double> AudioWaveformView::ConvertToDisplayData(const QByteArray &pcm_data, const QAudioFormat &format)
{
    QList<double> samples;
    const int bytes_per_sample = format.bytesPerSample();
    const int channel_count = format.channelCount();
    const int frame_size = bytes_per_sample * channel_count;
    // 只处理单声道或取立体声的左声道
    for (int i = 0; i < pcm_data.size(); i += frame_size) {
        if (i + bytes_per_sample > pcm_data.size()) {
            break;
        }
        double sample_value = 0.0;
        // 根据采样格式进行转换（仅支持当前AudioModel中的两种格式）
        switch (format.sampleFormat()) {
        case QAudioFormat::Int16: {
            const qint16 *data_ptr = reinterpret_cast<const qint16*>(pcm_data.constData() + i);
            sample_value = static_cast<double>(*data_ptr) / 32768.0; // 归一化到 [-1, 1]
            break;
        }
        case QAudioFormat::Float: {
            const float *data_ptr = reinterpret_cast<const float*>(pcm_data.constData() + i);
            sample_value = static_cast<double>(*data_ptr); // float格式已经是 [-1, 1]
            break;
        }
        default:
            sample_value = 0.0;
            break;
        }
        samples.append(sample_value);
    }
    return samples;
}

void AudioWaveformView::UpdateDisplay()
{
    if (sample_buffer_.isEmpty() || current_format_.sampleRate() <= 0) {
        return;
    }
    // 准备显示点
    QList<QPointF> points;
    points.reserve(sample_buffer_.size());
    // 简单的线性时间分布：将样本均匀分布在0到3秒的时间轴上
    const double time_per_sample = kDisplayDuration / kMaxDisplayPoints;
    for (int i = 0; i < sample_buffer_.size(); ++i) {
        const double time = i * time_per_sample;
        points.append(QPointF(time, sample_buffer_[i]));
    }
    // 更新波形数据
    waveform_series_->replace(points);
    // 保持X轴范围固定
    axis_x_->setRange(0.0, kDisplayDuration);
}

void AudioWaveformView::StartDisplay()
{
    is_displaying_ = true;
    ClearDisplay();
}

void AudioWaveformView::StopDisplay()
{
    is_displaying_ = false;
}

void AudioWaveformView::ClearDisplay()
{
    sample_buffer_.clear();
    waveform_series_->clear();
    axis_x_->setRange(0, kDisplayDuration);
}


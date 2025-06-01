#include "audiomodel.h"

AudioModel::AudioModel(QObject *parent)
    : QObject(parent)
{
    // 设置默认音频格式
    audio_format_.setSampleRate(44100);
    audio_format_.setChannelCount(1);
    audio_format_.setSampleFormat(QAudioFormat::Int16);
    // 选择默认设备
    current_device_ = QMediaDevices::defaultAudioInput();
}

AudioModel::~AudioModel()
{}

QStringList AudioModel::AvaiableAudioDevices() const
{
    QStringList device_names;
    const auto devices = QMediaDevices::audioInputs();
    for (const auto &device : devices) {
        device_names.append(device.description());
    }
    return device_names;
}

bool AudioModel::SetAudioSettings(const QString &device, const QString &format, const QString &sample_rate, const QString &channel)
{
    // 1. 设置音频设备
    const auto devices = QMediaDevices::audioInputs();
    for (const auto &d : devices) {
        if (d.description() == device) {
            current_device_ = d;
            break;
        }
    }
    // 2. 设置采样格式
    QAudioFormat::SampleFormat sf;
    if (format == "PCM 16-bit") {
        sf = QAudioFormat::Int16;
    } else if (format == "PCM float") {
        sf = QAudioFormat::Float;
    }
    audio_format_.setSampleFormat(sf);
    // 3. 设置采样率
    const auto sr = sample_rate.toInt();
    audio_format_.setSampleRate(sr);
    // 4. 设置声道数
    int ch{ 0 };
    if (channel.startsWith("1")) {
        ch = 1;
    } else if (channel.startsWith("2 ")) {
        ch = 2;
    }
    audio_format_.setChannelCount(ch);
    // 5. 检查音频格式是否可用
    if (!current_device_.isFormatSupported(audio_format_)) {
        return false;
    }
    return true;
}

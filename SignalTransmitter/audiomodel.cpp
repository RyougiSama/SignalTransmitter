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

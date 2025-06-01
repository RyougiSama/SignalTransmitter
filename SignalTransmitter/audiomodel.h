#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>

class AudioModel  : public QObject
{
    Q_OBJECT

public:
    AudioModel(QObject *parent);
    ~AudioModel();

    QStringList AvaiableAudioDevices() const;
    bool SetAudioSettings(const QString &device, const QString &format, const QString &sample_rate, const QString &channel);

private:
    QAudioDevice current_device_;
    QAudioFormat audio_format_;
};


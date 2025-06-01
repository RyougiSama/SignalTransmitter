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

private:
    QAudioDevice current_device_;
    QAudioFormat audio_format_;
};


#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QTimer>
#include <QFile>

class AudioModel  : public QObject
{
    Q_OBJECT

public:
    AudioModel(QObject *parent);
    ~AudioModel();

    QStringList AvaiableAudioDevices() const;
    bool SetAudioSettings(const QString &device, const QString &format, const QString &sample_rate, const QString &channel);
    bool StartRecording();
    void StopRecording();
    bool SaveRecordedWavFile(const QString &file_path) const;

    const QByteArray &get_recorded_data() const { return recorded_data_; }

private:
    void WriteWavHeader(QFile &file) const;

private:
    // 录音设备和格式设置
    QAudioDevice current_device_;
    QAudioFormat audio_format_;
    // 进行录音相关
    QAudioSource *audio_source_{ nullptr };
    QIODevice *audio_io_{ nullptr };
    QByteArray recorded_data_;
    QTimer *duration_timer_;
    int recording_duration_{ 0 };
    // 实时显示相关
    QByteArray temp_buffer_;

private slots:
    void SlotDurationUpdate() {
        recording_duration_++;
        emit RecordingDurationChanged(recording_duration_);
    }

signals:
    void RecordingDurationChanged(int seconds);
    void AudioDataReady(const QByteArray &data, const QAudioFormat &format);
};


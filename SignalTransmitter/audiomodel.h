#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QTimer>
#include <QFile>
#include <QAudioSink>
#include <QBuffer>

class AudioModel  : public QObject
{
    Q_OBJECT

public:
    AudioModel(QObject *parent);
    ~AudioModel();

    // 录音
    QStringList AvaiableAudioDevices() const;
    bool SetAudioSettings(const QString &device, const QString &format, const QString &sample_rate, const QString &channel);
    bool StartRecording();
    void StopRecording();
    bool SaveRecordedWavFile(const QString &file_path) const;
    // 播放
    bool LoadWavFile(const QString &file_path);
    bool StartPlayback();
    void StopPlayback();
    void PausePlayback();
    bool IsPlaying() const { return audio_sink_ && audio_sink_->state() == QAudio::ActiveState; }
    // 获取私有变量值
    const QByteArray &get_recorded_data() const { return recorded_data_; }
    int get_playback_total_duration() const { return playback_total_duration_; }

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
    // 播放相关
    QAudioSink *audio_sink_{ nullptr };
    QBuffer *playback_buffer_{ nullptr };
    QByteArray playback_data_;
    QAudioFormat playback_format_;
    QTimer *playback_timer_;
    int playback_total_duration_{ 0 };
    int playback_current_position_{ 0 };

private slots:
    // 录音波形更新
    void SlotDurationUpdate() {
        recording_duration_++;
        emit RecordingDurationChanged(recording_duration_);
    }
    // 播放进度更新
    void SlotPlaybackUpdate() {
        playback_current_position_++;
        emit PlaybackPositionChanged(playback_current_position_, playback_total_duration_);
        // 检查是否播放完成
        if (playback_current_position_ >= playback_total_duration_) {
            StopPlayback();
            emit PlaybackFinished();
        }
    }

signals:
    // 录音时长更新信号
    void RecordingDurationChanged(int seconds);
    // 录音波形数据更新信号
    void AudioDataReady(const QByteArray &data, const QAudioFormat &format);
    // 播放进度更新信号
    void PlaybackPositionChanged(int current_seconds, int total_seconds);
    // 播放完成信号
    void PlaybackFinished();
};

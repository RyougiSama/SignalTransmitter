#include "audiomodel.h"
#include "audiowaveformview.h"

AudioModel::AudioModel(QObject *parent)
    : QObject(parent)
    , duration_timer_(new QTimer(this))
    , playback_timer_(new QTimer(this))
{
    // 设置默认音频格式
    audio_format_.setSampleRate(44100);
    audio_format_.setChannelCount(1);
    audio_format_.setSampleFormat(QAudioFormat::Int16);
    // 选择默认设备
    current_device_ = QMediaDevices::defaultAudioInput();
    // 设置计时器
    connect(duration_timer_, &QTimer::timeout, this, &AudioModel::SlotDurationUpdate);
    connect(playback_timer_, &QTimer::timeout, this, &AudioModel::SlotPlaybackUpdate);
}

AudioModel::~AudioModel()
{
    StopRecording();
}

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
    auto sf{ QAudioFormat::Int16 };
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
    } else if (channel.startsWith("2")) {
        ch = 2;
    }
    audio_format_.setChannelCount(ch);
    // 5. 检查音频格式是否可用
    if (!current_device_.isFormatSupported(audio_format_)) {
        return false;
    }

    return true;
}

bool AudioModel::StartRecording()
{
    if (audio_source_) {
        delete audio_source_;
        audio_source_ = nullptr;
    }
    audio_source_ = new QAudioSource(current_device_, audio_format_, this);
    if (!audio_source_) {
        return false;
    }
    recorded_data_.clear();
    temp_buffer_.clear();
    recording_duration_ = 0;
    // 开始录音
    audio_io_ = audio_source_->start();
    if (!audio_io_) {
        delete audio_source_;
        audio_source_ = nullptr;
        return false;
    }
    connect(audio_io_, &QIODevice::readyRead, [this]() {
        if (audio_io_) {
            const QByteArray new_data = audio_io_->readAll();
            if (!new_data.isEmpty()) {
                // 保存到录音数据
                recorded_data_.append(new_data);
                // 添加到实时显示缓冲区
                temp_buffer_.append(new_data);
                // 检查是否有足够数据进行实时显示
                const int bytes_per_sample = audio_format_.bytesPerSample();
                const int frame_size = bytes_per_sample * audio_format_.channelCount();
                const int target_samples = audio_format_.sampleRate() * AudioWaveformView::kDisplayDuration;
                const int target_bytes = target_samples * frame_size;
                if (temp_buffer_.size() >= target_bytes) {
                    // 发射实时数据信号
                    emit AudioDataReady(temp_buffer_, audio_format_);
                    // 清空缓冲区
                    temp_buffer_.clear();
                }
            }
        }
    });
    // 启动定时器
    duration_timer_->start(1000);
    return true;
}

void AudioModel::StopRecording()
{
    // 停止计时器
    duration_timer_->stop();
    // 停止录音
    if (audio_source_) {
        audio_source_->stop();
        delete audio_source_;
        audio_source_ = nullptr;
    }
    audio_io_ = nullptr;
    // 清空临时缓冲区
    temp_buffer_.clear();
}

bool AudioModel::SaveRecordedWavFile(const QString &file_path) const
{
    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    // 写入 WAV 头部信息
    WriteWavHeader(file);
    // 写入录音数据
    const auto bytes_written = file.write(recorded_data_);
    file.close();

    return bytes_written == recorded_data_.size();
}

void AudioModel::WriteWavHeader(QFile &file) const
{
    // WAV 文件头部信息
    struct WavHeader {
        char riff[4] = {'R', 'I', 'F', 'F'}; // "RIFF"
        uint32_t file_size; // 文件大小 - 8
        char wave[4] = {'W', 'A', 'V', 'E'}; // "WAVE"
        char fmt[4] = {'f', 'm', 't', ' '}; // "fmt "
        uint32_t fmt_size = 16; // fmt chunk size
        uint16_t audio_format = 1; // 1: PCM format 3: float format
        uint16_t num_channels; // 声道数
        uint32_t sample_rate; // 采样率
        uint32_t byte_rate; // 每秒字节数
        uint16_t block_align; // 块对齐
        uint16_t bits_per_sample; // 每个样本的位数
        char data[4] = {'d', 'a', 't', 'a'}; // "data"
        uint32_t data_size; // 数据大小
    };

    WavHeader header;
    header.num_channels = audio_format_.channelCount();
    header.sample_rate = audio_format_.sampleRate();
    header.bits_per_sample = audio_format_.bytesPerSample() * 8;
    header.byte_rate = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    header.block_align = header.num_channels * (header.bits_per_sample / 8);
    header.data_size = recorded_data_.size();
    header.file_size = sizeof(WavHeader) - 8 + header.data_size;
    // 根据Qt音频格式设置WAV格式字段
    switch (audio_format_.sampleFormat()) {
    case QAudioFormat::Int16:
        header.audio_format = 1; // PCM
        break;
    case QAudioFormat::Float:
        header.audio_format = 3; // IEEE float
        break;
    default:
        header.audio_format = 1; // 默认PCM
        break;
    }
    file.write(reinterpret_cast<const char *>(&header), sizeof(WavHeader));
}

bool AudioModel::LoadWavFile(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    // WAV文件头解析
    struct WavHeader {
        char riff[4];
        uint32_t file_size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        char data[4];
        uint32_t data_size;
    };
    WavHeader header;
    // 读取WAV头部信息，检查返回读取的字节数是否正确
    if (file.read(reinterpret_cast<char *>(&header), sizeof(WavHeader)) != sizeof(WavHeader)) {
        return false;
    }
    // 验证WAV格式
    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        return false;
    }
    // 设置播放格式
    playback_format_.setChannelCount(header.num_channels);
    playback_format_.setSampleRate(header.sample_rate);
    playback_format_.setSampleFormat(header.bits_per_sample == 16 ? QAudioFormat::Int16 : QAudioFormat::Float);
    // 读取音频数据
    playback_data_ = file.read(header.data_size);
    // 计算总时长（秒）
    const int bytes_per_second = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    playback_total_duration_ = playback_data_.size() / bytes_per_second;

    return !playback_data_.isEmpty();
}

bool AudioModel::StartPlayback()
{
    if (playback_data_.isEmpty()) {
        return false;
    }
    // 获取默认音频输出设备
    const auto output_device = QMediaDevices::defaultAudioOutput();
    if (!output_device.isFormatSupported(playback_format_)) {
        return false;
    }
    // 创建音频输出
    audio_sink_ = new QAudioSink(output_device, playback_format_, this);
    // 创建播放缓冲区
    playback_buffer_ = new QBuffer(&playback_data_, this);
    playback_buffer_->open(QIODevice::ReadOnly);
    // 开始播放
    audio_sink_->start(playback_buffer_);
    // 启动进度定时器
    playback_current_position_ = 0;
    playback_timer_->start(1000); // 每秒更新一次进度

    return true;
}

void AudioModel::StopPlayback()
{
    playback_timer_->stop();
    if (audio_sink_) {
        audio_sink_->stop();
        delete audio_sink_;
        audio_sink_ = nullptr;
    }
    if (playback_buffer_) {
        playback_buffer_->close();
        delete playback_buffer_;
        playback_buffer_ = nullptr;
    }
    playback_current_position_ = 0;
    emit PlaybackPositionChanged(0, playback_total_duration_);
}

void AudioModel::PausePlayback()
{
    if (audio_sink_->state() == QAudio::ActiveState) {
        audio_sink_->suspend();
        playback_timer_->stop();
    } else if (audio_sink_->state() == QAudio::SuspendedState) {
        audio_sink_->resume();
        playback_timer_->start(1000);
    }
}

#include "audiomodel.h"

AudioModel::AudioModel(QObject *parent)
    : QObject(parent)
    , duration_timer_(new QTimer(this))
{
    // 设置默认音频格式
    audio_format_.setSampleRate(44100);
    audio_format_.setChannelCount(1);
    audio_format_.setSampleFormat(QAudioFormat::Int16);
    // 选择默认设备
    current_device_ = QMediaDevices::defaultAudioInput();
    // 设置计时器
    connect(duration_timer_, &QTimer::timeout, this, &AudioModel::SlotDurationUpdate);
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
            recorded_data_.append(audio_io_->readAll());
        }
            });
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
    audio_source_ = nullptr;
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

#include "txtmodel.h"
#include <QFile>
#include <QMessageBox>

TxtModel::TxtModel(QObject *parent)
    : QObject(parent)
{}

TxtModel::~TxtModel()
{}

bool TxtModel::LoadTxtFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return false;
    }
    QTextStream in(&file);
    txt_raw_data_ = in.readAll();
    file.close();
    return true;
}

bool TxtModel::SaveTxtFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << txt_raw_data_;
    file.close();
    return true;
}

void TxtModel::EncodeTxtFile(const QString &encode_t)
{
    txt_encoded_data_.clear();
    QByteArray encoded;

    // 根据编码类型将原始文本转换为字节流
    if (encode_t.compare("UTF-8", Qt::CaseInsensitive) == 0) {
        encoded = txt_raw_data_.toUtf8();
    } else if (encode_t.compare("UTF-16", Qt::CaseInsensitive) == 0) {
        const auto *data = reinterpret_cast<const char16_t *>(txt_raw_data_.utf16());
        auto byte_count = txt_raw_data_.size() * 2;
        encoded = QByteArray(reinterpret_cast<const char *>(data), byte_count);
    }
    // 按位展开，每个位存为0或1
    txt_encoded_data_.reserve(encoded.size() * 8);
    for (auto byte : encoded) {
        auto ubyte = static_cast<uint8_t>(static_cast<unsigned char>(byte));
        for (auto i{ 7 }; i >= 0; --i) { // 高位在前
            txt_encoded_data_.append((ubyte >> i) & 0x01);
        }
    }
}

void TxtModel::ModulateTxtFile(const QString &modulate_t)
{
    const double ask_high{ 1.0 }, ask_low{ 0.0 };       // ASK高低电平
    // 载波参数
    const double ask_f0 = kCarrierFreq;    // ASK载波频率
    const double psk_f = kCarrierFreq;     // PSK载波频率

    const auto sample_rate{ kSampleRate };
    const auto samples_per_bit{ kSamplesPerBit };

    // 计算预期数据大小并预分配内存
    const qsizetype expected_size = txt_encoded_data_.size() * samples_per_bit;
    txt_modulated_data.clear();
    txt_modulated_data.reserve(expected_size);

    if (modulate_t.compare("ASK", Qt::CaseInsensitive) == 0) {
        // 振幅键控
        // 预计算一个比特周期内的正弦波形状
        QList<double> high_pattern(samples_per_bit);
        QList<double> low_pattern(samples_per_bit);

        // 计算高电平和低电平的波形模板
        for (qsizetype n = 0; n < samples_per_bit; ++n) {
            double t = static_cast<double>(n) / sample_rate;
            high_pattern[n] = ask_high * sin(2 * M_PI * ask_f0 * t);
            low_pattern[n] = ask_low * sin(2 * M_PI * ask_f0 * t);
        }

        // 对每个比特应用预计算的波形模板
        for (auto bit : txt_encoded_data_) {
            const auto& pattern = (bit == 1) ? high_pattern : low_pattern;
            txt_modulated_data.append(pattern);
        }
    } else if (modulate_t.compare("PSK", Qt::CaseInsensitive) == 0) {
        // 相位键控
        // 预计算一个比特周期内的0相位和π相位正弦波形状
        QList<double> phase0_pattern(samples_per_bit);
        QList<double> phasePI_pattern(samples_per_bit);

        // 计算两种相位的波形模板
        for (qsizetype n = 0; n < samples_per_bit; ++n) {
            double t = static_cast<double>(n) / sample_rate;
            phase0_pattern[n] = sin(2 * M_PI * psk_f * t);
            phasePI_pattern[n] = sin(2 * M_PI * psk_f * t + M_PI);
        }

        // 对每个比特应用预计算的波形模板
        for (auto bit : txt_encoded_data_) {
            const auto& pattern = (bit == 1) ? phasePI_pattern : phase0_pattern;
            txt_modulated_data.append(pattern);
        }
    }
}

void TxtModel::SaveEncodedFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return;
    }
    QTextStream out(&file);
    for (auto bit : txt_encoded_data_) {
        out << bit;
    }
    file.close();
}

void TxtModel::SaveModulatedFile(const QString &file_name)
{
    QFile file(file_name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Cannot open file: %1")
                             .arg(file.errorString()));
        return;
    }
    QTextStream out(&file);
    for (auto sample : txt_modulated_data) {
        out << sample << " ";
    }
    file.close();
}

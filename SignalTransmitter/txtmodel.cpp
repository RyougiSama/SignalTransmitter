#include "txtmodel.h"
#include <QFile>
#include <QMessageBox>

TxtModel::TxtModel(QObject *parent)
    : QObject(parent)
{}

TxtModel::~TxtModel()
{}

bool TxtModel::LoadTxtFile(const QString &file_path)
{
    QFile file(file_path);
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

bool TxtModel::SaveTxtFile(const QString &file_path)
{
    QFile file(file_path);
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
    // 载波参数
    const double ask_high{ 1.0 }, ask_low{ 0.0 };       // ASK高低电平
    const double fsk_f0{ 1000.0 };
    const double psk_f{ 1000.0 };    // PSK载波频率
    const auto sample_rate{ kSampleRate };
    const auto samples_per_bit{ kSamplesPerBit };

    txt_modulated_data.clear();

    if (modulate_t.compare("ASK", Qt::CaseInsensitive) == 0) {
        // 振幅键控
        for (auto bit : txt_encoded_data_) {
            auto amplitude = (bit == 1) ? ask_high : ask_low;
            for (auto n{ 0 }; n < samples_per_bit; ++n) {
                auto t = n / sample_rate;
                auto value = amplitude * sin(2 * M_PI * fsk_f0 * t);
                txt_modulated_data.append(value);
            }
        }
    } else if (modulate_t.compare("PSK", Qt::CaseInsensitive) == 0) {
        // 相位键控
        for (auto bit : txt_encoded_data_) {
            auto phase = (bit == 1) ? M_PI : 0.0;
            for (auto n{ 0 }; n < samples_per_bit; ++n) {
                auto t = n / sample_rate;
                auto value = sin(2 * M_PI * psk_f * t + phase);
                txt_modulated_data.append(value);
            }
        }
    }
}


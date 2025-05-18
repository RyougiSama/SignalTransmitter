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

bool TxtModel::EncodeTxtFile(const QString &encode_t)
{
#ifdef QT_DEBUG
    qDebug() << "Encoding file to" << encode_t;
#endif

    txt_encoded_data_.clear();
    QByteArray encoded;

    // 根据编码类型将原始文本转换为字节流
    if (encode_t.compare("UTF-8", Qt::CaseInsensitive) == 0) {
        encoded = txt_raw_data_.toUtf8();
    } else if (encode_t.compare("UTF-16", Qt::CaseInsensitive) == 0) {
        const char16_t *data = reinterpret_cast<const char16_t *>(txt_raw_data_.utf16());
        int byteCount = txt_raw_data_.size() * 2;
        encoded = QByteArray(reinterpret_cast<const char *>(data), byteCount);
    } else {
        QMessageBox::warning(static_cast<QWidget *>(parent()), "Error", QString("Unsupported encoding: %1").arg(encode_t));
        return false;
    }

    // 按位展开，每个位存为0或1
    txt_encoded_data_.reserve(encoded.size() * 8);
    for (auto byte : encoded) {
        uint8_t ubyte = static_cast<uint8_t>(static_cast<unsigned char>(byte));
        for (int i = 7; i >= 0; --i) { // 高位在前
            txt_encoded_data_.append((ubyte >> i) & 0x01);
        }
    }
    return true;
}


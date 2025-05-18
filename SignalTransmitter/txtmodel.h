#pragma once

#include <QObject>
#include <QList>

class TxtModel  : public QObject
{
    Q_OBJECT

public:
    TxtModel(QObject *parent);
    ~TxtModel();

    QString get_txt_raw_data() const { return txt_raw_data_; }
    const QList<uint8_t> &get_txt_encoded_data() const { return txt_encoded_data_; }

    bool LoadTxtFile(const QString &file_path);
    bool SaveTxtFile(const QString &file_path);
    bool EncodeTxtFile(const QString &encode_t);

private:
    QString txt_raw_data_;
    QList<uint8_t> txt_encoded_data_;
};

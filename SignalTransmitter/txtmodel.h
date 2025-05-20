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
    const QList<double> &get_txt_modulated_data() const { return txt_modulated_data; }

    bool LoadTxtFile(const QString &file_name);
    bool SaveTxtFile(const QString &file_name);
    void EncodeTxtFile(const QString &encode_t);
    void ModulateTxtFile(const QString &modulate_t);
    void SaveEncodedFile(const QString &file_name);
    void SaveModulatedFile(const QString &file_name);

    static constexpr double kSampleRate{ 800.0 };
    static constexpr qsizetype kSamplesPerBit { 5 };

private:
    QString txt_raw_data_;
    QList<uint8_t> txt_encoded_data_;
    QList<double> txt_modulated_data;
};

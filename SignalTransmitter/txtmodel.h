#pragma once

#include <QObject>

class TxtModel  : public QObject
{
    Q_OBJECT

public:
    TxtModel(QObject *parent);
    ~TxtModel();

    QString get_txt_raw_data() const { return txt_raw_data_; }

    bool LoadTxtFile(const QString &file_path);
    bool SaveTxtFile(const QString &file_path);

private:
    QString txt_raw_data_;
};

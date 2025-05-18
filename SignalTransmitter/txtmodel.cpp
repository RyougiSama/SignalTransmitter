#include "txtmodel.h"
#include "QFile"
#include "QMessageBox"

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


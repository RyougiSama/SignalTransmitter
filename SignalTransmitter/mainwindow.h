#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "txtmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindowClass *ui;
    TxtModel *txt_model_ = nullptr;

private slots:
    void on_btn_load_txt_clicked();
    void on_btn_save_txt_clicked();
};

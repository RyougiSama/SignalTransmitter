#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "txtmodel.h"
#include "networkmodel.h"
#include "audiomodel.h"

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
    void InitAudioSettings();

private:
    Ui::MainWindowClass *ui;
    TxtModel *txt_model_;
    NetworkModel *network_model_;
    AudioModel *audio_model_;

private slots:
    void on_btn_load_txt_clicked();
    void on_btn_save_txt_clicked();
    void on_btn_encode_clicked();
    void on_btn_modulate_clicked();
    void on_btn_save_encoded_file_clicked();
    void on_btn_save_modulated_file_clicked();
    void on_btn_port_listening_clicked(bool isChecked);
    void on_btn_load_trans_file_clicked();
    void on_btn_start_trans_clicked();
    void on_btn_refresh_devices_clicked();
};

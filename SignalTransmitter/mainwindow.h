#pragma once

#include <QtWidgets/QWidget>
#include "ui_mainwindow.h"
#include "txtmodel.h"
#include "networkmodel.h"
#include "audiomodel.h"
#include "audiowaveformview.h"

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
    // 文本模型
    void on_btn_load_txt_clicked();
    void on_btn_save_txt_clicked();
    void on_btn_encode_clicked();
    void on_btn_modulate_clicked();
    void on_btn_save_encoded_file_clicked();
    void on_btn_save_modulated_file_clicked();
    // 网络模型
    void on_btn_port_listening_clicked(bool isChecked);
    void on_btn_load_trans_file_clicked();
    void on_btn_start_trans_clicked();
    // 音频模型
    void on_btn_refresh_devices_clicked();
    void on_btn_record_switch_clicked(bool isChecked);
    void on_btn_save_recorded_file_clicked();
    void on_btn_open_recorded_file_clicked();
    void on_btn_play_wav_clicked();
    void on_btn_pause_wav_clicked();
    void on_btn_close_wav_clicked();

    void UpdatePlaybackProgress(int current_seconds, int total_seconds);
    void OnPlaybackFinished();
};

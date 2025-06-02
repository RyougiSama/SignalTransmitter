#include "mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , txt_model_(new TxtModel(this))
    , network_model_(new NetworkModel(this))
    , audio_model_(new AudioModel(this))
{
    ui->setupUi(this);
    ui->label_sample_rate->setText("采样率: " + QString::number(txt_model_->kSampleRate) + " Hz"
    + " 传信率: " + QString::number(txt_model_->kSampleRate / txt_model_->kSamplesPerBit) + " bps"
    + " 载波: " + QString::number(txt_model_->kCarrierFreq) + " Hz");
    ui->time_view_encoded->set_txt_model(txt_model_);
    ui->time_view_modulated->set_txt_model(txt_model_);
    // 初始化音频设置
    InitAudioSettings();
    // 连接音频模型的录音时长信号
    connect(audio_model_, &AudioModel::RecordingDurationChanged, [this](int seconds) {
        int minutes = seconds / 60;
        int secs = seconds % 60;
        ui->label_recording_duration->setText(QString("录音时长: %1:%2")
                                              .arg(minutes, 2, 10, QChar('0'))
                                              .arg(secs, 2, 10, QChar('0')));
            });
    // 连接音频模型的实时数据信号到波形显示
    connect(audio_model_, &AudioModel::AudioDataReady, ui->audio_waveform_view, &AudioWaveformView::UpdateWaveform);
    // 连接播放进度信号
    connect(audio_model_, &AudioModel::PlaybackPositionChanged, this, &MainWindow::UpdatePlaybackProgress);
    connect(audio_model_, &AudioModel::PlaybackFinished, this, &MainWindow::OnPlaybackFinished);
    // 连接网络模型的信号到槽
    connect(network_model_, &NetworkModel::connectionEstablished, [this](const QString &client_info) {
        ui->textBrowser_link_info->append("连接已建立");
        ui->textBrowser_link_info->append(QString("客户端: %1").arg(client_info));
    });
    connect(network_model_, &NetworkModel::transferProgress, [this](qint64 bytes_sent, qint64 total_bytes) {
        static int last_progress = -1;
        int progress = static_cast<int>((bytes_sent * 100) / total_bytes);
        // 只在进度变化超过1%时更新显示，避免过于频繁的更新
        if (progress != last_progress && (progress - last_progress >= 1 || progress == 100)) {
            ui->textBrowser_link_info->append(QString("传输进度: %1% (%2/%3 字节)")
                                             .arg(progress)
                                             .arg(bytes_sent)
                                             .arg(total_bytes));
            last_progress = progress;
        }
    });
    connect(network_model_, &NetworkModel::transferCompleted, [this]() {
        ui->textBrowser_link_info->append("文件传输完成");
        ui->btn_start_trans->setEnabled(true);
    });
    connect(network_model_, &NetworkModel::transferError, [this](const QString &error_message) {
        ui->textBrowser_link_info->append("传输错误: " + error_message);
        ui->btn_start_trans->setEnabled(true);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 初始化音频设置
void MainWindow::InitAudioSettings()
{
    // 初始化录音设备下拉框
    ui->comboBox_audio_devices->clear();
    const auto devices = audio_model_->AvaiableAudioDevices();
    for (const auto &device : devices) {
        ui->comboBox_audio_devices->addItem(device);
    }
}

// 按钮槽函数实现
void MainWindow::on_btn_load_txt_clicked()
{
    auto file_name = QFileDialog::getOpenFileName(this, "Open TXT File", "", "Text Files (*.txt)");
    if (!file_name.isEmpty()) {
        if (txt_model_->LoadTxtFile(file_name)) {
            ui->textBrowser_txt->setText(txt_model_->get_txt_raw_data());
            ui->btn_encode->setEnabled(true);
            ui->btn_save_txt->setEnabled(true);
        }
    }
}

void MainWindow::on_btn_save_txt_clicked()
{
    QString file_name = QFileDialog::getSaveFileName(this, "Save TXT File", "", "Text Files (*.txt)");
    if (!file_name.isEmpty()) {
        txt_model_->SaveTxtFile(file_name);
    }
}

void MainWindow::on_btn_encode_clicked()
{
    txt_model_->EncodeTxtFile(ui->comboBox_encoding->currentText());
    // 以二进制形式显示编码后的数据，每个样本点为0或1
    const auto &data = txt_model_->get_txt_encoded_data();
    QString bin_str;
    for (auto bit : data) {
        bin_str.append(QString::number(bit));
    }
    ui->textBrowser_encoded->setText(bin_str.trimmed());
    ui->btn_modulate->setEnabled(true);
    ui->btn_save_encoded_file->setEnabled(true);
    // 更新编码波形
    ui->time_view_encoded->UpdateView();
}

void MainWindow::on_btn_modulate_clicked()
{
    txt_model_->ModulateTxtFile(ui->comboBox_modulation->currentText());
    const auto &data = txt_model_->get_txt_modulated_data();
    QString mod_str;
    for (auto bit : data) {
        mod_str.append(QString::number(bit, 'f', 2)); // 保留两位小数
        mod_str.append(" ");
    }
    ui->textBrowser_modulated->setText(mod_str.trimmed());
    ui->btn_save_modulated_file->setEnabled(true);
    // 更新调制波形
    ui->time_view_modulated->set_modulation_type(ui->comboBox_modulation->currentText());
    ui->time_view_modulated->UpdateView();
}

void MainWindow::on_btn_save_encoded_file_clicked()
{
    const auto file_name = QFileDialog::getSaveFileName(this, "Save Encoded File", "", "Text Files (*.txt)");
    if (!file_name.isEmpty()) {
        txt_model_->SaveEncodedFile(file_name);
    }
}

void MainWindow::on_btn_save_modulated_file_clicked()
{
    const auto file_name = QFileDialog::getSaveFileName(this, "Save Modulated File", "", "Text Files (*.txt)");
    if (!file_name.isEmpty()) {
        txt_model_->SaveModulatedFile(file_name);
    }
}

void MainWindow::on_btn_port_listening_clicked(bool isChecked)
{
    if (isChecked) {
        const auto port_str = ui->lineEdit_port->text();
        ui->lineEdit_port->setEnabled(false);
        ui->btn_port_listening->setText("端口监听中...");
        ui->textBrowser_link_info->append("当前监听端口号: " + port_str);

        switch (network_model_->StartListening(port_str)) {
        case NetworkModel::kNoError:
            ui->textBrowser_link_info->append("端口监听成功");
            break;
        case NetworkModel::kPortConversionError:
            QMessageBox::warning(this, "端口转换错误", "无法将端口号转换为整数，请检查输入的端口号是否有效。");
            ui->lineEdit_port->setEnabled(true);
            ui->btn_port_listening->setText("开始监听端口");
            break;
        case NetworkModel::kPortStartError:
            QMessageBox::warning(this, "端口启动错误", "无法启动端口监听，请检查端口是否被占用或其他网络问题。");
            ui->lineEdit_port->setEnabled(true);
            ui->btn_port_listening->setText("开始监听端口");
            break;
        default:
            break;
        }
    } else {
        ui->lineEdit_port->setEnabled(true);
        ui->btn_port_listening->setText("开始监听端口");
        ui->textBrowser_link_info->append("端口监听已停止");

        network_model_->StopListening();
    }
}

void MainWindow::on_btn_load_trans_file_clicked()
{
    const auto file_name = QFileDialog::getOpenFileName(this, "Open Transmit File", "", "Text Files (*.txt)");
    ui->lineEdit_trans_file_path->setText(file_name);
    if (file_name.isEmpty()) {
        return;
    }
    network_model_->set_preview_file(file_name);
    ui->textBrowser_txt_preview->setText(network_model_->get_preview_file());
}

void MainWindow::on_btn_start_trans_clicked()
{
    const auto file_name = ui->lineEdit_trans_file_path->text();
    if (file_name.isEmpty()) {
        QMessageBox::warning(this, "文件未选择", "请先选择要传输的文件。");
        return;
    }
    if (network_model_->get_port_state() == NetworkModel::kNotListening) {
        QMessageBox::warning(this, "端口未监听", "请先启动端口监听。");
        return;
    }
    if (network_model_->get_transfer_state() == NetworkModel::kTransferring) {
        QMessageBox::warning(this, "传输进行中", "文件正在传输中，请等待完成。");
        return;
    }
    // 开始传输文件
    ui->btn_start_trans->setEnabled(false);
    ui->textBrowser_link_info->append("开始传输文件: " + file_name);
    network_model_->StartFileTransfer(file_name);
}

void MainWindow::on_btn_refresh_devices_clicked()
{
    InitAudioSettings();
}

void MainWindow::on_btn_record_switch_clicked(bool isChecked)
{
    if (isChecked) {
        const auto device = ui->comboBox_audio_devices->currentText();
        const auto format = ui->comboBox_sample_format->currentText();
        const auto sample_rate = ui->comboBox_sample_rate->currentText();
        const auto channel = ui->comboBox_channel_count->currentText();

        if (device.isEmpty()) {
            QMessageBox::warning(this, "录音错误", "请选择一个有效的录音设备。");
            ui->btn_record_switch->setChecked(false);
            return;
        }
        if (!audio_model_->SetAudioSettings(device, format, sample_rate, channel)) {
            QMessageBox::warning(this, "参数设置失败", "无法应用所选的音频参数。请检查设备兼容性或参数选择。");
            ui->btn_record_switch->setChecked(false);
            return;
        }
        // 开始录音
        if (!audio_model_->StartRecording()) {
            QMessageBox::warning(this, "录音启动失败", "无法启动录音功能，请检查音频设备状态。");
            ui->btn_record_switch->setChecked(false);
        }
        // 开始波形显示
        ui->audio_waveform_view->StartDisplay();

        ui->label_recording_duration->setText(QString("录音时长: 00:00"));
        ui->btn_record_switch->setText("停止录音");
        // 禁用参数选择控件
        ui->comboBox_audio_devices->setEnabled(false);
        ui->comboBox_sample_format->setEnabled(false);
        ui->comboBox_sample_rate->setEnabled(false);
        ui->comboBox_channel_count->setEnabled(false);
        ui->btn_refresh_devices->setEnabled(false);    } else {
        // 停止录音
        audio_model_->StopRecording();
        // 停止波形显示
        ui->audio_waveform_view->StopDisplay();
        // 显示录音完成信息
        const auto recorded_data = audio_model_->get_recorded_data();
        if (!recorded_data.isEmpty()) {
            QMessageBox::information(this, "录音完成",
                                     QString("录音已保存到缓冲区，数据大小: %1 KB").arg(recorded_data.size() / 1024.0, 0, 'f', 2));
        }

        ui->btn_record_switch->setText("开始录音");
        // 启用参数选择控件
        ui->comboBox_audio_devices->setEnabled(true);
        ui->comboBox_sample_format->setEnabled(true);
        ui->comboBox_sample_rate->setEnabled(true);
        ui->comboBox_channel_count->setEnabled(true);
        ui->btn_refresh_devices->setEnabled(true);
    }
}

void MainWindow::on_btn_save_recorded_file_clicked()
{
    if (ui->btn_record_switch->isChecked()) {
        QMessageBox::warning(this, "保存失败", "录音正在进行中，请先停止录音。");
        return;
    }
    const auto recorded_data = audio_model_->get_recorded_data();
    if (recorded_data.isEmpty()) {
        QMessageBox::warning(this, "保存失败", "没有录音数据可以保存。");
        return;
    }
    const auto file_name = QFileDialog::getSaveFileName(this, "保存录音文件", "", "WAV Files (*.wav)");
    if (file_name.isEmpty()) {
        return;
    }

    if (audio_model_->SaveRecordedWavFile(file_name)) {
        QMessageBox::information(this, "保存成功",
                                 QString("录音文件已保存: %1").arg(file_name));
    } else {
        QMessageBox::warning(this, "保存失败", "无法保存录音文件，请检查文件路径和权限。");
    }
}

void MainWindow::on_btn_open_recorded_file_clicked()
{
    const auto file_name = QFileDialog::getOpenFileName(this, "Open WAV File", "", "WAV Files (*.wav)");
    if (file_name.isEmpty()) {
        return;
    }
    ui->lineEdit_wav_file_path->setText(file_name);
    if (audio_model_->LoadWavFile(file_name)) {
        const int total_duration = audio_model_->get_playback_total_duration();
        const int minutes = total_duration / 60;
        const int seconds = total_duration % 60;

        ui->label_playback->setText(QString("00:00 / %1:%2")
                                    .arg(minutes, 2, 10, QChar('0'))
                                    .arg(seconds, 2, 10, QChar('0')));
        ui->progressBar_playback->setValue(0);

        QMessageBox::information(this, "文件加载成功",
                                 QString("WAV文件已加载: %1\n时长: %2:%3")
                                 .arg(QFileInfo(file_name).fileName())
                                 .arg(minutes, 2, 10, QChar('0'))
                                 .arg(seconds, 2, 10, QChar('0')));
    } else {
        ui->lineEdit_wav_file_path->clear();
        QMessageBox::warning(this, "文件加载失败", "无法加载WAV文件，请检查文件格式。");
    }
}

void MainWindow::on_btn_play_wav_clicked()
{
    if (ui->lineEdit_wav_file_path->text().isEmpty()) {
        QMessageBox::warning(this, "播放失败", "请先选择要播放的WAV文件。");
        return;
    }
    if (audio_model_->StartPlayback()) {
        ui->btn_play_wav->setEnabled(false);
        ui->btn_pause_wav->setEnabled(true);
        ui->btn_close_wav->setEnabled(true);
        ui->btn_open_recorded_file->setEnabled(false);
    } else {
        QMessageBox::warning(this, "播放失败", "无法开始播放，请检查音频文件和设备状态。");
    }
}

void MainWindow::on_btn_pause_wav_clicked()
{
    audio_model_->PausePlayback();
    if (audio_model_->IsPlaying()) {
        ui->btn_pause_wav->setText("暂停");
    } else {
        ui->btn_pause_wav->setText("继续");
    }
}

void MainWindow::on_btn_close_wav_clicked()
{
    audio_model_->StopPlayback();
    OnPlaybackFinished();
}

void MainWindow::UpdatePlaybackProgress(int current_seconds, int total_seconds)
{
    const int current_minutes = current_seconds / 60;
    const int current_secs = current_seconds % 60;
    const int total_minutes = total_seconds / 60;
    const int total_secs = total_seconds % 60;
    // 更新时间标签
    ui->label_playback->setText(QString("%1:%2 / %3:%4")
                                .arg(current_minutes, 2, 10, QChar('0'))
                                .arg(current_secs, 2, 10, QChar('0'))
                                .arg(total_minutes, 2, 10, QChar('0'))
                                .arg(total_secs, 2, 10, QChar('0')));
    // 更新进度条
    if (total_seconds > 0) {
        const int progress = (current_seconds * 100) / total_seconds;
        ui->progressBar_playback->setValue(progress);
    }
}

void MainWindow::OnPlaybackFinished()
{
    ui->btn_play_wav->setEnabled(true);
    ui->btn_pause_wav->setEnabled(false);
    ui->btn_close_wav->setEnabled(false);
    ui->btn_pause_wav->setText("暂停");
    ui->btn_open_recorded_file->setEnabled(true);
    ui->progressBar_playback->setValue(0);

    const int total_duration = audio_model_->get_playback_total_duration();
    const int minutes = total_duration / 60;
    const int seconds = total_duration % 60;
    ui->label_playback->setText(QString("00:00 / %1:%2")
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0')));
}

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

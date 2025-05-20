#include "mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , txt_model_(new TxtModel(this))
{
    ui->setupUi(this);
    ui->label_sample_rate->setText("采样率: " + QString::number(txt_model_->kSampleRate) + " Hz");
    ui->time_view_encoded->set_txt_model(txt_model_);
}

MainWindow::~MainWindow()
{
    delete ui;
}

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
}


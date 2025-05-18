#include "mainwindow.h"
#include "QFileDialog"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindowClass())
    , txt_model_(new TxtModel(this))
{
    ui->setupUi(this);
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
    if (txt_model_->EncodeTxtFile(ui->comboBox_encoding->currentText())) {
        // 以二进制形式显示编码后的数据，每个样本点为0或1
        const auto &data = txt_model_->get_txt_encoded_data();
        QString binStr;
        for (uint8_t bit : data) {
            binStr.append(QString::number(bit));
        }
        ui->textBrowser_encoded->setText(binStr.trimmed());
        ui->btn_modulate->setEnabled(true);
    } else {
        QMessageBox::warning(this, "Encoding Error", "Failed to encode the text.");
    }
}


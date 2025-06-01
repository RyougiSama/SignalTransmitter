#include "networkmodel.h"
#include <QFileInfo>
#include <QMessageBox>

NetworkModel::NetworkModel(QObject *parent)
    : QObject(parent)
    , server_(new QTcpServer(this))
{
    connect(server_, &QTcpServer::newConnection, this, &NetworkModel::SlotNewConnection);
}

NetworkModel::~NetworkModel()
{}

NetworkModel::PortError_t NetworkModel::StartListening(const QString &port)
{
    bool ok{ false };
    const auto port_number = port.toUShort(&ok);
    if (!ok || port_number <= 0) {
        return kPortConversionError;
    }
    if (!server_->listen(QHostAddress::Any, port_number)) {
        return kPortStartError;
    }
    port_state_ = kIsListening;
    return kNoError;
}

void NetworkModel::StopListening()
{
    if (server_->isListening()) {
        server_->close();
        port_state_ = kNotListening;
    }
}

void NetworkModel::SlotNewConnection()
{
    if (server_->hasPendingConnections()) {
        socket_ = server_->nextPendingConnection();
        connect(socket_, &QTcpSocket::disconnected, this, &NetworkModel::SlotSocketDisconnected);
        connect(socket_, &QTcpSocket::bytesWritten, this, &NetworkModel::SlotBytesWritten);
        const auto client_info = QString("%1:%2")
            .arg(socket_->peerAddress().toString())
            .arg(socket_->peerPort());
        emit connectionEstablished(client_info);
    }
}

void NetworkModel::StartFileTransfer(const QString &file_path)
{
    if (!socket_ || socket_->state() != QTcpSocket::ConnectedState) {
        emit transferError("无连接的客户端");
        return;
    }
    if (transfer_file_) {
        transfer_file_->close();
        delete transfer_file_;
    }
    transfer_file_ = new QFile(file_path, this);
    if (!transfer_file_->open(QIODevice::ReadOnly)) {
        emit transferError("无法打开文件: " + file_path);
        delete transfer_file_;
        transfer_file_ = nullptr;
        return;
    }

    QFileInfo file_info(file_path);
    total_bytes_ = file_info.size();
    bytes_written_ = 0;
    file_bytes_written_ = 0;
    transfer_state_ = kTransferring;

    // 发送文件头信息：文件名长度 + 文件名 + 文件大小
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    QString file_name = file_info.fileName();
    stream << file_name << total_bytes_;

    header_size_ = header.size();
    socket_->write(header);

    // 开始发送文件内容
    SendNextChunk();
}

void NetworkModel::SendNextChunk()
{
    if (!transfer_file_ || !socket_) {
        return;
    }

    const qint64 chunk_size = 64 * 1024; // 64KB块大小
    QByteArray chunk = transfer_file_->read(chunk_size);

    if (chunk.isEmpty()) {
        // 文件传输完成
        transfer_state_ = kTransferCompleted;
        transfer_file_->close();
        delete transfer_file_;
        transfer_file_ = nullptr;
        emit transferCompleted();
        return;
    }

    qint64 bytes_written = socket_->write(chunk);
    if (bytes_written == -1) {
        transfer_state_ = kTransferError;
        emit transferError("写入套接字失败");
        return;
    }
}

void NetworkModel::SlotSocketDisconnected()
{
    if (transfer_state_ == kTransferring) {
        transfer_state_ = kTransferError;
        emit transferError("客户端连接断开");
    }

    if (transfer_file_) {
        transfer_file_->close();
        delete transfer_file_;
        transfer_file_ = nullptr;
    }

    // 重置传输状态
    total_bytes_ = 0;
    bytes_written_ = 0;
    header_size_ = 0;
    file_bytes_written_ = 0;

    socket_ = nullptr;
}

void NetworkModel::SlotBytesWritten(qint64 bytes)
{
    if (transfer_state_ != kTransferring) {
        return;
    }

    bytes_written_ += bytes;

    // 如果还在发送头部数据，不更新文件传输进度
    if (bytes_written_ <= header_size_) {
        return;
    }

    // 计算实际的文件数据传输字节数
    file_bytes_written_ = bytes_written_ - header_size_;

    // 确保不超过文件大小
    if (file_bytes_written_ > total_bytes_) {
        file_bytes_written_ = total_bytes_;
    }

    emit transferProgress(file_bytes_written_, total_bytes_);

    // 继续发送下一块
    if (file_bytes_written_ < total_bytes_) {
        SendNextChunk();
    }
}

void NetworkModel::set_preview_file(const QString &file_path)
{
    preview_file_.clear();

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        preview_file_ = QString("无法打开文件: %1").arg(file.errorString());
        return;
    }

    QTextStream in(&file);
    preview_file_ = in.readAll();
    file.close();
}

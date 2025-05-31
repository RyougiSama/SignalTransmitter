#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>

class NetworkModel  : public QObject
{
    Q_OBJECT

public:
    enum PortState_t {
        kIsListening,
        kNotListening
    };

    enum PortError_t {
        kNoError,
        kPortConversionError,
        kPortStartError
    };

    enum TransferState_t {
        kIdle,
        kTransferring,
        kTransferCompleted,
        kTransferError
    };

public:
    NetworkModel(QObject *parent);
    ~NetworkModel();

    PortState_t get_port_state() const { return port_state_; }
    PortError_t StartListening(const QString &port);
    void StopListening();

    void StartFileTransfer(const QString &file_path);
    TransferState_t get_transfer_state() const { return transfer_state_; }

private:
    QTcpServer *server_;
    QTcpSocket *socket_{ nullptr };
    QFile *transfer_file_{ nullptr };    PortState_t port_state_{ kNotListening };
    TransferState_t transfer_state_{ kIdle };
    qint64 total_bytes_{ 0 };
    qint64 bytes_written_{ 0 };
    qint64 header_size_{ 0 };
    qint64 file_bytes_written_{ 0 };

    void SendNextChunk();

private slots:
    void SlotNewConnection();
    void SlotSocketDisconnected();
    void SlotBytesWritten(qint64 bytes);

signals:
    void connectionEstablished();
    void transferProgress(qint64 bytes_sent, qint64 total_bytes);
    void transferCompleted();
    void transferError(const QString &error_message);
};

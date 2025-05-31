#pragma once

#include <QObject>
#include <QTcpServer>

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

public:
    NetworkModel(QObject *parent);
    ~NetworkModel();

    PortState_t get_port_state() const { return port_state_; }

    PortError_t StartListening(const QString &port);
    void StopListening();

private:
    QTcpServer *server_;
    QTcpSocket *socket_{ nullptr };

    PortState_t port_state_{ kNotListening };

private slots:
    void SlotNewConnection();

signals:
    void connectionEstablished();
};

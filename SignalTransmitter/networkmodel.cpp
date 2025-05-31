#include "networkmodel.h"

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
        emit connectionEstablished();
    }
}

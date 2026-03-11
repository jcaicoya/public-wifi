#include "TcpJsonLineServer.h"

#include <QString>

TcpJsonLineServer::TcpJsonLineServer(quint16 port, QObject* parent)
    : QObject(parent), m_port(port)
{
    connect(&m_server, &QTcpServer::newConnection,
            this, &TcpJsonLineServer::onNewConnection);
}

bool TcpJsonLineServer::start()
{
    if (!m_server.listen(QHostAddress::Any, m_port)) {
        emit errorOccurred(QString("Failed to listen on port %1: %2")
                               .arg(m_port)
                               .arg(m_server.errorString()));
        return false;
    }
    return true;
}

void TcpJsonLineServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket* socket = m_server.nextPendingConnection();
        if (!socket)
            continue;

        const QString peer =
            QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

        emit clientConnected(peer);

        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            onSocketReadyRead(socket);
        });

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            onSocketDisconnected(socket);
        });

        connect(socket, &QTcpSocket::errorOccurred, this,
         [this, socket](QAbstractSocket::SocketError error) {
            if (error == QAbstractSocket::RemoteHostClosedError) {
                return; // Normal in our current router scripts: one connection per event
            }

            emit errorOccurred(QString("Socket error on port %1: %2")
                            .arg(m_port)
                            .arg(socket->errorString()));
        });
    }
}

void TcpJsonLineServer::onSocketReadyRead(QTcpSocket* socket)
{
    QByteArray& buffer = m_buffers[socket];
    buffer += socket->readAll();

    while (true) {
        const int newlinePos = buffer.indexOf('\n');
        if (newlinePos < 0)
            break;

        QByteArray line = buffer.left(newlinePos);
        buffer.remove(0, newlinePos + 1);

        if (!line.trimmed().isEmpty()) {
            emit lineReceived(line.trimmed());
        }
    }
}

void TcpJsonLineServer::onSocketDisconnected(QTcpSocket* socket)
{
    // Process any remaining data in the buffer before closing
    if (m_buffers.contains(socket)) {
        QByteArray& buffer = m_buffers[socket];
        if (!buffer.isEmpty()) {
            // If the buffer doesn't end with a newline, we might have a partial line.
            // In our protocol, we'll try to process it as a final line if it's not empty.
            if (buffer.indexOf('\n') < 0) {
                if (!buffer.trimmed().isEmpty()) {
                    emit lineReceived(buffer.trimmed());
                }
            } else {
                // There are still newlines to process
                onSocketReadyRead(socket);
            }
        }
    }

    const QString peer =
        QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

    emit clientDisconnected(peer);

    m_buffers.remove(socket);
    socket->deleteLater();
}

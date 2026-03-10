#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>

class TcpJsonLineServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpJsonLineServer(quint16 port, QObject* parent = nullptr);

    bool start();
    quint16 port() const { return m_port; }

    signals:
        void lineReceived(const QByteArray& line);
    void clientConnected(const QString& peer);
    void clientDisconnected(const QString& peer);
    void errorOccurred(const QString& message);

private:
    void onNewConnection();
    void onSocketReadyRead(QTcpSocket* socket);
    void onSocketDisconnected(QTcpSocket* socket);

    quint16 m_port;
    QTcpServer m_server;
    QHash<QTcpSocket*, QByteArray> m_buffers;
};

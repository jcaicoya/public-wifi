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
#include <QShortcut>
#include <QKeySequence>
#include <QScrollBar>
#include <QTcpSocket>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>

#include <QRandomGenerator>

namespace
{
QString pageName(MainWindow::PageId pageId)
{
    switch (pageId) {
    case MainWindow::PageId::Main: return "Main";
    case MainWindow::PageId::Devices: return "Devices";
    case MainWindow::PageId::Navigation: return "Navigation";
    case MainWindow::PageId::Statistics: return "Statistics";
    case MainWindow::PageId::Encryption: return "Encryption";
    }
    return "?";
}
}

MainWindow::MainWindow(bool demoMode, QWidget* parent)
    : QMainWindow(parent), m_isDemoMode(demoMode)
{
    buildUi();
    wireNavigation();
    goTo(PageId::Main);

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

    if (!m_trafficServer->start()) {
        qWarning() << "Traffic server failed to start";
    }

    if (!m_deviceServer->start()) {
        qWarning() << "Device server failed to start";
    }

    if (m_isDemoMode) {
        if (m_console1) m_console1->setPlainText("Running in DEMO MODE...\nNo router connection required.");
        if (m_console2) m_console2->setPlainText("Running in DEMO MODE...\nWaiting for simulated events.");
        if (m_console3) m_console3->setPlainText("Running in DEMO MODE...\nLogs disabled.");
        if (m_console4) m_console4->setPlainText("Running in DEMO MODE...\nConnections hidden.");
        startDemoMode();
        if (m_demoBtnC) {
            m_demoBtnC->setText("■  Stop Demo");
            m_demoBtnC->setStyleSheet(
                "QPushButton { background:#2b0d0d; color:#FF3333; border:1px solid #FF3333; "
                "padding:6px 14px; font-family:Consolas; font-size:11px; font-weight:bold; }"
                "QPushButton:hover { background:#401a1a; }"
            );
        }
    } else {
        m_routerRetryTimer = new QTimer(this);
        connect(m_routerRetryTimer, &QTimer::timeout, this, &MainWindow::tryConnectRouter);
        tryConnectRouter();
    }
}

MainWindow::~MainWindow()
{
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
    }

    // En C mostramos solo el tráfico del device seleccionado
    if (m_filteredTrafficViewC) {
        QString selectedIp;
        if (m_devicesListB && m_devicesListB->currentItem()) {
            selectedIp = m_devicesListB->currentItem()->toolTip();
        }

        if (selectedIp.isEmpty() || selectedIp == ip) {
            m_filteredTrafficViewC->append(
                QString("%1  %2  %3").arg(device.isEmpty() ? ip : device, event, domain));
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
    m_pageA = new ScreenPage("", "Cybershow Command Center", this);

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

    auto* statusLabel = new QLabel("SYSTEM STATUS", controlsWidget);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #FF3333; font-weight: bold; font-family: Consolas; font-size: 16px;");

    auto* imageLabel = new QLabel(controlsWidget);
    QPixmap cuarzito(":/flying-cuarzito.png");
    imageLabel->setPixmap(cuarzito.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setAlignment(Qt::AlignCenter);

    controlsLayout->addStretch();
    controlsLayout->addWidget(introLabel);
    controlsLayout->addWidget(statusLabel);
    controlsLayout->addSpacing(40);
    controlsLayout->addWidget(imageLabel);
    controlsLayout->addStretch();

    mainSplitter->addWidget(consolesWidget);
    mainSplitter->addWidget(controlsWidget);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);

    m_pageA->contentLayout()->addWidget(mainSplitter);

    auto* toA = m_pageA->addNavButton("Main");
    auto* toB = m_pageA->addNavButton("Devices");
    auto* toC = m_pageA->addNavButton("Navigation");
    auto* toD = m_pageA->addNavButton("Statistics");
    auto* toE = m_pageA->addNavButton("Encryption");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::Main); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::Devices); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::Navigation); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::Statistics); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::Encryption); });
}

void MainWindow::buildPageB()
{
    m_pageB = new ScreenPage("", "Devices + Raw Traffic", this);

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

    auto* toA = m_pageB->addNavButton("Main");
    auto* toB = m_pageB->addNavButton("Devices");
    auto* toC = m_pageB->addNavButton("Navigation");
    auto* toD = m_pageB->addNavButton("Statistics");
    auto* toE = m_pageB->addNavButton("Encryption");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::Main); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::Devices); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::Navigation); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::Statistics); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::Encryption); });
}

void MainWindow::buildPageC()
{
    m_pageC = new ScreenPage("", "No device selected", this);

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

    m_demoBtnC = new QPushButton("▶  Demo Mode", eventsPane);
    m_demoBtnC->setStyleSheet(
        "QPushButton { background:#0d2b10; color:#00FF44; border:1px solid #00FF44; "
        "padding:6px 14px; font-family:Consolas; font-size:11px; font-weight:bold; }"
        "QPushButton:hover { background:#1a4020; }"
    );
    connect(m_demoBtnC, &QPushButton::clicked, this, [this]() {
        if (m_demoTimer) {
            m_demoTimer->stop();
            m_demoTimer->deleteLater();
            m_demoTimer = nullptr;
            m_demoBtnC->setText("▶  Demo Mode");
            m_demoBtnC->setStyleSheet(
                "QPushButton { background:#0d2b10; color:#00FF44; border:1px solid #00FF44; "
                "padding:6px 14px; font-family:Consolas; font-size:11px; font-weight:bold; }"
                "QPushButton:hover { background:#1a4020; }"
            );
            statusBar()->showMessage("Demo mode stopped.", 2000);
        } else {
            startDemoMode();
            if (m_demoTimer) {
                m_demoBtnC->setText("■  Stop Demo");
                m_demoBtnC->setStyleSheet(
                    "QPushButton { background:#2b0d0d; color:#FF3333; border:1px solid #FF3333; "
                    "padding:6px 14px; font-family:Consolas; font-size:11px; font-weight:bold; }"
                    "QPushButton:hover { background:#401a1a; }"
                );
            }
        }
    });

    m_filteredTrafficViewC = new QTextEdit(eventsPane);
    m_filteredTrafficViewC->setReadOnly(true);

    eventsLayout->addWidget(eventsTitle);
    eventsLayout->addWidget(m_demoBtnC);
    eventsLayout->addWidget(m_filteredTrafficViewC, 1);

    splitter->addWidget(mapPane);
    splitter->addWidget(eventsPane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    m_pageC->contentLayout()->addWidget(splitter);

    auto* toA = m_pageC->addNavButton("Main");
    auto* toB = m_pageC->addNavButton("Devices");
    auto* toC = m_pageC->addNavButton("Navigation");
    auto* toD = m_pageC->addNavButton("Statistics");
    auto* toE = m_pageC->addNavButton("Encryption");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::Main); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::Devices); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::Navigation); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::Statistics); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::Encryption); });
}

void MainWindow::buildPageD()
{
    m_pageD = new ScreenPage("", "Statistics / History", this);

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

    auto* toA = m_pageD->addNavButton("Main");
    auto* toB = m_pageD->addNavButton("Devices");
    auto* toC = m_pageD->addNavButton("Navigation");
    auto* toD = m_pageD->addNavButton("Statistics");
    auto* toE = m_pageD->addNavButton("Encryption");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::Main); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::Devices); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::Navigation); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::Statistics); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::Encryption); });
}

void MainWindow::buildPageE()
{
    m_pageE = new ScreenPage("", "Encryption Analysis", this);

    m_hackerTerminalE = new QTextEdit(m_pageE);
    m_hackerTerminalE->setReadOnly(true);
    m_hackerTerminalE->setStyleSheet("background: #090C10; color: #00FF44; font-family: Consolas, monospace; font-size: 14px; border: 1px solid #1E283C;");
    m_hackerTerminalE->setPlainText("Waiting for target data stream...");
    
    m_lockedPlaceholderE = new QLabel(m_pageE);
    m_lockedPlaceholderE->setAlignment(Qt::AlignCenter);
    
    // Set the reassuring text and styling
    m_lockedPlaceholderE->setText(
        "CRITICAL ERROR: DECRYPTION FAILED\n\n"
        "Estimated time to crack: 13.8 Billion Years\n"
        "STATUS: END-TO-END ENCRYPTED. YOUR SECRETS ARE SAFE."
    );
    
    QFont font = m_lockedPlaceholderE->font();
    font.setPointSize(24);
    font.setBold(true);
    m_lockedPlaceholderE->setFont(font);
    
    // Use a reassuring cyan/blue border instead of angry red
    m_lockedPlaceholderE->setStyleSheet("color: #00FFFF; background: #090C10; border: 3px solid #00FFFF; border-radius: 10px; padding: 30px;");
    m_lockedPlaceholderE->hide(); // Hidden initially

    m_pageE->contentLayout()->addWidget(m_hackerTerminalE, 2);
    m_pageE->contentLayout()->addSpacing(20);
    m_pageE->contentLayout()->addWidget(m_lockedPlaceholderE, 1);

    auto* toA = m_pageE->addNavButton("Main");
    auto* toB = m_pageE->addNavButton("Devices");
    auto* toC = m_pageE->addNavButton("Navigation");
    auto* toD = m_pageE->addNavButton("Statistics");
    auto* toE = m_pageE->addNavButton("Encryption");
    auto* startDemoBtn = m_pageE->addNavButton("Trigger Demo");

    connect(toA, &QPushButton::clicked, this, [this]() { goTo(PageId::Main); });
    connect(toB, &QPushButton::clicked, this, [this]() { goTo(PageId::Devices); });
    connect(toC, &QPushButton::clicked, this, [this]() { goTo(PageId::Navigation); });
    connect(toD, &QPushButton::clicked, this, [this]() { goTo(PageId::Statistics); });
    connect(toE, &QPushButton::clicked, this, [this]() { goTo(PageId::Encryption); });
    connect(startDemoBtn, &QPushButton::clicked, this, &MainWindow::startEncryptionDemo);

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
    m_hackerTerminalE->append("> INITIATING DEEP PACKET INSPECTION\n");

    if (m_isDemoMode) {
        m_hackerTerminalE->append("> Intercepted packet from target device to whatsapp.net");
        m_hackerTerminalE->append("> Extracting raw payload...\n");
        m_hackerTerminalE->append(generateHexPayload(15));
        m_hackerTerminalE->append("\n> Payload captured. Content is unreadable (Encrypted).");
        m_hackerTerminalE->append("> Initializing Brute-Force Decryption Cluster...");
        
        m_encryptionStep = 0;
        m_bruteForceTick = 0;
        m_encryptionTimer->start(50);
    } else {
        QString targetIp;
        if (m_devicesListB && m_devicesListB->currentItem()) {
            targetIp = m_devicesListB->currentItem()->toolTip();
        }

        if (targetIp.isEmpty()) {
            m_hackerTerminalE->append("> ERROR: No target device selected.");
            m_hackerTerminalE->append("> Please select a device on the Devices screen first.");
            return;
        }

        m_hackerTerminalE->append(QString("> Sniffing real-time traffic for IP: %1").arg(targetIp));
        m_hackerTerminalE->append("> Filter: Port 443 (HTTPS) / 5222 (WhatsApp)");
        m_hackerTerminalE->append("> WAITING FOR TARGET TO SEND DATA...\n");

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
                m_hackerTerminalE->append("\n> Interception complete.");
                m_hackerTerminalE->append("> Payload captured. Content is unreadable (Encrypted).");
                m_hackerTerminalE->append("> Initializing Brute-Force Decryption Cluster...");
                
                m_encryptionStep = 0;
                m_bruteForceTick = 0;
                m_encryptionTimer->start(50);
            } else {
                m_hackerTerminalE->append("\n> Interception aborted or timed out.");
            }
        });

        QStringList args;
        args << "root@192.168.8.1" << QString("/root/sniff_payload.sh %1").arg(targetIp);
        m_sniffProc->start("ssh", args);
    }
}

void MainWindow::updateEncryptionAnimation()
{
    if (!m_hackerTerminalE) return;

    if (m_encryptionStep == 0) {
        // Phase 1: Rapid key injection simulation (lasts about 3 seconds)
        if (m_bruteForceTick % 2 == 0) {
            m_hackerTerminalE->append(QString("> Injecting key stream: [%1]").arg(generateHexPayload(1).trimmed()));
        }
        
        m_bruteForceTick++;
        if (m_bruteForceTick > 60) {
            m_encryptionStep = 1;
            m_bruteForceTick = 0;
            m_hackerTerminalE->append("\n> Key dictionary exhausted.");
            m_hackerTerminalE->append("> Switching to AES-256-GCM brute-force...");
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
        bar += QString("] %1% - CPU LOAD: 100%").arg(progress);
        
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
        m_hackerTerminalE->append("\n> FATAL: Forward secrecy confirmed.");
        m_hackerTerminalE->append("> Mathematical limits reached.");
        m_hackerTerminalE->append("> Aborting operation.\n");
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

    // Global keyboard shortcuts that cannot be intercepted by child widgets
    new QShortcut(QKeySequence(Qt::Key_1), this, [this]() { goTo(PageId::Main); });
    new QShortcut(QKeySequence(Qt::Key_M), this, [this]() { goTo(PageId::Main); });

    new QShortcut(QKeySequence(Qt::Key_2), this, [this]() { goTo(PageId::Devices); });
    new QShortcut(QKeySequence(Qt::Key_D), this, [this]() { goTo(PageId::Devices); });

    new QShortcut(QKeySequence(Qt::Key_3), this, [this]() { goTo(PageId::Navigation); });
    new QShortcut(QKeySequence(Qt::Key_N), this, [this]() { goTo(PageId::Navigation); });

    new QShortcut(QKeySequence(Qt::Key_4), this, [this]() { goTo(PageId::Statistics); });
    new QShortcut(QKeySequence(Qt::Key_S), this, [this]() { goTo(PageId::Statistics); });

    new QShortcut(QKeySequence(Qt::Key_5), this, [this]() { goTo(PageId::Encryption); });
    new QShortcut(QKeySequence(Qt::Key_E), this, [this]() { goTo(PageId::Encryption); });

    new QShortcut(QKeySequence(Qt::Key_Left), this, [this]() {
        int currentIndex = m_stack->currentIndex();
        if (currentIndex > 0) {
            goTo(static_cast<PageId>(currentIndex - 1));
        }
    });

    new QShortcut(QKeySequence(Qt::Key_Right), this, [this]() {
        int currentIndex = m_stack->currentIndex();
        if (currentIndex < m_stack->count() - 1) {
            goTo(static_cast<PageId>(currentIndex + 1));
        }
    });
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
        m_pageC->setTitle("No device selected");
    } else {
        QString title = QString("TARGET: %1 (%2)").arg(selectedName, selectedIp);
        if (!selectedMac.isEmpty()) {
            title += QString(" - MAC: %1").arg(selectedMac);
        }
        m_pageC->setTitle(title);
    }
}

void MainWindow::updateStatsView()
{
    if (!m_statsPlaceholderD) return;

    QString selectedIp;
    QString selectedName;
    if (m_devicesListB && m_devicesListB->currentItem()) {
        selectedIp = m_devicesListB->currentItem()->toolTip();
        selectedName = m_devicesListB->currentItem()->text();
    }

    if (selectedIp.isEmpty()) {
        m_statsPlaceholderD->setPlainText("No device selected.\n\nSelect a device on Screen B to view its statistics.");
        return;
    }

    if (!m_deviceStats.contains(selectedIp)) {
        m_statsPlaceholderD->setPlainText(QString("No statistics available yet for: %1").arg(selectedName));
        return;
    }

    const auto& stats = m_deviceStats[selectedIp];
    
    QString text = QString("=== TARGET PROFILE: %1 ===\n").arg(selectedName);
    text += QString("Target IP: %1\n\n").arg(selectedIp);
    
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
}

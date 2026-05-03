#pragma once

#include <QMainWindow>
#include <QHash>
#include <QJsonArray>
#include <QDateTime>
#include "ShowConfig.h"
#include "cybershow/common/ScreenDefinition.h"

class QEvent;
class QKeyEvent;

class QGraphicsOpacityEffect;
class QProgressBar;
class QPropertyAnimation;
class QStackedWidget;
class QListWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QTimer;

class BottomNavBar;
class ScreenPage;
class TcpJsonLineServer;
class MapView;
class QProcess;
class WifiPortalServer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const ShowConfig& config, QWidget* parent = nullptr);
    ~MainWindow() override;

    enum class PageId
    {
        Main = 0,
        Devices,
        Navigation,
        Statistics,
        Encryption
    };

signals:
    void setupRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct DeviceStats {
        int totalEvents = 0;
        QHash<QString, int> serviceCounts;
        QDateTime firstSeen;
        QDateTime lastSeen;
    };

    void buildUi();
    void buildPageA();
    void buildPageB();
    void buildPageC();
    void buildPageD();
    void buildPageE();

    void wireNavigation();
    void goTo(PageId pageId);
    void goToScreenNumber(int number);
    void goToAdjacentScreen(int direction);
    bool handleRuntimeKeyPress(QKeyEvent* event);
    bool focusIsEditable(QWidget* focusWidget) const;

    QString getLocalIpAddress() const;
    void processTrafficEvent(const QByteArray& rawLine, const QJsonObject& obj);
    void processDeviceEvent(const QJsonObject& obj);
    void processCredentialEvent(const QString& name, const QString& email);
    void updateStatsView();
    void updateNavigationHeader();

private slots:
    void startRouterScripts();
    void stopRouterScripts();
    void startDemoMode();
    void onDemoTimerTick();
    void tryConnectRouter();

private:
    ShowConfig m_config;
    cybershow::ScreenDefinitions m_screens;
    QTimer* m_routerRetryTimer  = nullptr;
    QTimer* m_actSequenceTimer  = nullptr;
    int     m_actSequenceIndex  = 0;
    QTimer* m_demoSyslogTimer   = nullptr;
    int     m_syslogLine        = 0;

    QStackedWidget*       m_stack             = nullptr;
    BottomNavBar*         m_bottomNav         = nullptr;
    QWidget*              m_transitionOverlay = nullptr;
    QPropertyAnimation*   m_transitionAnim    = nullptr;

    ScreenPage* m_pageA = nullptr;
    ScreenPage* m_pageB = nullptr;
    ScreenPage* m_pageC = nullptr;
    ScreenPage* m_pageD = nullptr;
    ScreenPage* m_pageE = nullptr;

    // A
    QTextEdit* m_console1 = nullptr;
    QTextEdit* m_console2 = nullptr;
    QTextEdit* m_console3 = nullptr;
    QTextEdit* m_console4 = nullptr;
    
    QProcess* m_sshProc1 = nullptr;
    QProcess* m_sshProc2 = nullptr;
    QProcess* m_sshProc3 = nullptr;
    QProcess* m_sshProc4 = nullptr;
    QProcess* m_sniffProc = nullptr;

    void startSshConsole(QProcess*& proc, QTextEdit* console, const QString& command);

    // B
    QListWidget* m_devicesListB = nullptr;
    QTextEdit* m_rawTrafficViewB = nullptr;
    QLabel* m_portalUrlLabelB = nullptr;
    QLabel* m_credentialBannerB = nullptr;

    // C
    MapView* m_mapViewC = nullptr;
    QTextEdit* m_filteredTrafficViewC = nullptr;

    // D
    QLabel*       m_scoreLabelD       = nullptr;
    QLabel*       m_riskLabelD        = nullptr;
    QProgressBar* m_scoreBarD         = nullptr;
    QTextEdit*    m_statsPlaceholderD = nullptr;

    // E
    QLabel* m_lockedPlaceholderE = nullptr;
    QTextEdit* m_hackerTerminalE = nullptr;
    QTimer* m_encryptionTimer = nullptr;
    int m_encryptionStep = 0;
    int m_bruteForceTick = 0;
    
    void startEncryptionDemo();
    void updateEncryptionAnimation();
    QString generateHexPayload(int lines);

    // TCP servers
    TcpJsonLineServer* m_trafficServer = nullptr;
    TcpJsonLineServer* m_deviceServer = nullptr;
    WifiPortalServer*  m_portalServer  = nullptr;

    // Demo Mode state
    QTimer* m_demoTimer = nullptr;
    QJsonArray m_demoEvents;
    int m_demoIndex = 0;
    
    // Stats
    QHash<QString, DeviceStats> m_deviceStats;
};

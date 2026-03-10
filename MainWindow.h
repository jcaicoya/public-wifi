#pragma once

#include <QMainWindow>
#include <QHash>
#include <QJsonArray>

class QStackedWidget;
class QListWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QTimer;

class ScreenPage;
class TcpJsonLineServer;
class MapView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    enum class PageId
    {
        A = 0,
        B,
        C,
        D,
        E
    };

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildUi();
    void buildPageA();
    void buildPageB();
    void buildPageC();
    void buildPageD();
    void buildPageE();

    void wireNavigation();
    void goTo(PageId pageId);

    QString getLocalIpAddress() const;
    void processTrafficEvent(const QByteArray& rawLine, const QJsonObject& obj);
    void processDeviceEvent(const QJsonObject& obj);

private slots:
    void startRouterScripts();
    void stopRouterScripts();
    void startDemoMode();
    void onDemoTimerTick();

private:
    QStackedWidget* m_stack = nullptr;

    ScreenPage* m_pageA = nullptr;
    ScreenPage* m_pageB = nullptr;
    ScreenPage* m_pageC = nullptr;
    ScreenPage* m_pageD = nullptr;
    ScreenPage* m_pageE = nullptr;

    // B
    QListWidget* m_devicesListB = nullptr;
    QTextEdit* m_rawTrafficViewB = nullptr;

    // C
    MapView* m_mapViewC = nullptr;
    QTextEdit* m_filteredTrafficViewC = nullptr;

    // D
    QTextEdit* m_statsPlaceholderD = nullptr;

    // E
    QLabel* m_lockedPlaceholderE = nullptr;

    // Optional next-step integration
    TcpJsonLineServer* m_trafficServer = nullptr;
    TcpJsonLineServer* m_deviceServer = nullptr;

    // Demo Mode state
    QTimer* m_demoTimer = nullptr;
    QJsonArray m_demoEvents;
    int m_demoIndex = 0;
};
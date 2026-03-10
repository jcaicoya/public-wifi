#pragma once

#include <QMainWindow>
#include <QHash>

class QStackedWidget;
class QListWidget;
class QTextEdit;
class QLabel;
class QPushButton;

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

private slots:
    void startRouterScripts();
    void stopRouterScripts();
    void startDemoMode();

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
};
#pragma once

#include "CyberAppMode.h"
#include "ScreenDefinition.h"

#include <QObject>
#include <QKeyEvent>
#include <QWidget>

namespace cybershow {

class CyberNavigationController : public QObject {
    Q_OBJECT

public:
    CyberNavigationController(AppLaunchOptions options, ScreenDefinitions screens, QObject* parent = nullptr);

    int currentIndex() const;
    int currentScreenNumber() const;
    QString currentScreenId() const;
    const ScreenDefinitions& screens() const;

    bool handleSetupKeyPress(QKeyEvent* event, QWidget* focusWidget);
    bool handleRuntimeKeyPress(QKeyEvent* event, QWidget* focusWidget);

public slots:
    void goToFirstRuntimeScreen();
    void goToScreenNumber(int number);
    void nextScreen();
    void previousScreen();
    void returnToSetupIfAllowed();

signals:
    void startRequested();
    void setupRequested();
    void screenRequested(int index, cybershow::ScreenDefinition screen);
    void fullscreenToggleRequested();
    void debugOverlayToggleRequested();
    void resetCurrentScreenRequested();

private:
    bool focusIsEditable(QWidget* focusWidget) const;

    AppLaunchOptions m_options;
    ScreenDefinitions m_screens;
    int m_currentIndex = 0;
};

} // namespace cybershow

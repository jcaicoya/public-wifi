#include "CyberNavigationController.h"

#include <QComboBox>
#include <QAbstractSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <utility>

namespace cybershow {

CyberNavigationController::CyberNavigationController(AppLaunchOptions options, ScreenDefinitions screens, QObject* parent)
    : QObject(parent), m_options(std::move(options)), m_screens(std::move(screens)) {}

int CyberNavigationController::currentIndex() const { return m_currentIndex; }
int CyberNavigationController::currentScreenNumber() const { return m_screens.value(m_currentIndex).number; }
QString CyberNavigationController::currentScreenId() const { return m_screens.value(m_currentIndex).id; }
const ScreenDefinitions& CyberNavigationController::screens() const { return m_screens; }

bool CyberNavigationController::focusIsEditable(QWidget* focusWidget) const {
    if (!focusWidget) return false;
    if (qobject_cast<QLineEdit*>(focusWidget)) return true;
    if (qobject_cast<QTextEdit*>(focusWidget)) return true;
    if (qobject_cast<QPlainTextEdit*>(focusWidget)) return true;
    if (qobject_cast<QAbstractSpinBox*>(focusWidget)) return true;
    if (auto combo = qobject_cast<QComboBox*>(focusWidget)) return combo->isEditable();
    return false;
}

bool CyberNavigationController::handleSetupKeyPress(QKeyEvent* event, QWidget* focusWidget) {
    if (!event || focusIsEditable(focusWidget)) return false;

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Right:
    case Qt::Key_1:
        emit startRequested();
        return true;
    case Qt::Key_F11:
        emit fullscreenToggleRequested();
        return true;
    default:
        break;
    }

    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        if (event->key() == Qt::Key_D && m_options.launchMode == LaunchMode::Configure && m_options.debug) {
            emit debugOverlayToggleRequested();
            return true;
        }
    }

    return false;
}

bool CyberNavigationController::handleRuntimeKeyPress(QKeyEvent* event, QWidget* focusWidget) {
    if (!event || focusIsEditable(focusWidget)) return false;

    switch (event->key()) {
    case Qt::Key_Right:
        nextScreen();
        return true;
    case Qt::Key_Left:
        previousScreen();
        return true;
    case Qt::Key_Escape:
        returnToSetupIfAllowed();
        return true;
    case Qt::Key_F11:
        emit fullscreenToggleRequested();
        return true;
    default:
        break;
    }

    if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        goToScreenNumber(event->key() - Qt::Key_0);
        return true;
    }

    if (event->modifiers().testFlag(Qt::ControlModifier) && m_options.launchMode == LaunchMode::Configure) {
        if (event->key() == Qt::Key_D && m_options.debug) {
            emit debugOverlayToggleRequested();
            return true;
        }
        if (event->key() == Qt::Key_R) {
            emit resetCurrentScreenRequested();
            return true;
        }
    }

    return false;
}

void CyberNavigationController::goToFirstRuntimeScreen() {
    if (m_screens.isEmpty()) return;
    m_currentIndex = 0;
    emit screenRequested(m_currentIndex, m_screens[m_currentIndex]);
}

void CyberNavigationController::goToScreenNumber(int number) {
    const int index = indexForScreenNumber(m_screens, number);
    if (index < 0) return;
    m_currentIndex = index;
    emit screenRequested(m_currentIndex, m_screens[m_currentIndex]);
}

void CyberNavigationController::nextScreen() {
    if (m_screens.isEmpty()) return;
    int next = m_currentIndex;
    while (next + 1 < m_screens.size()) {
        ++next;
        if (m_screens[next].enabled) {
            m_currentIndex = next;
            emit screenRequested(m_currentIndex, m_screens[m_currentIndex]);
            return;
        }
    }
    // No circular navigation by default.
}

void CyberNavigationController::previousScreen() {
    if (m_screens.isEmpty()) return;
    int previous = m_currentIndex;
    while (previous - 1 >= 0) {
        --previous;
        if (m_screens[previous].enabled) {
            m_currentIndex = previous;
            emit screenRequested(m_currentIndex, m_screens[m_currentIndex]);
            return;
        }
    }
    // No circular navigation by default.
}

void CyberNavigationController::returnToSetupIfAllowed() {
    if (setupAvailable(m_options)) {
        emit setupRequested();
    }
}

} // namespace cybershow

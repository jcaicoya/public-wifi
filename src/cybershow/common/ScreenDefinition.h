#pragma once

#include <QString>
#include <QVector>

namespace cybershow {

enum class ScreenKind {
    Operative,
    Scenic
};

struct ScreenDefinition {
    int number = 1;
    QString id;
    QString title;
    QString shortTitle;
    ScreenKind kind = ScreenKind::Operative;
    bool enabled = true;
};

using ScreenDefinitions = QVector<ScreenDefinition>;

inline int indexForScreenNumber(const ScreenDefinitions& screens, int number) {
    for (int i = 0; i < screens.size(); ++i) {
        if (screens[i].number == number && screens[i].enabled) {
            return i;
        }
    }
    return -1;
}

} // namespace cybershow

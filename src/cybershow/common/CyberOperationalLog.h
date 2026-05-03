#pragma once

#include <QString>

namespace cybershow {

class OperationalLog {
public:
    static void configure(const QString& launchMode, const QString& profile);
    static void write(const QString& level, const QString& component, const QString& message);
    static QString filePath();
};

} // namespace cybershow

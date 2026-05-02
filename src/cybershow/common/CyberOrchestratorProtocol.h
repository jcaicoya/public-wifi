#pragma once

#include <QString>
#include <QTextStream>

namespace cybershow {

class OrchestratorProtocol {
public:
    static void status(const QString& status, const QString& detail = QString()) {
        QTextStream out(stdout);
        out << "CYBERSHOW_STATUS " << status;
        if (!detail.isEmpty()) out << " " << sanitize(detail);
        out << Qt::endl;
    }

    static void screen(int number, const QString& id) {
        QTextStream out(stdout);
        out << "CYBERSHOW_SCREEN " << number << " " << sanitize(id) << Qt::endl;
    }

    static void log(const QString& level, const QString& component, const QString& message) {
        QTextStream out(stdout);
        out << "CYBERSHOW_LOG " << sanitize(level) << " " << sanitize(component) << " " << sanitize(message) << Qt::endl;
    }

    static void event(const QString& name, const QString& payload = QString()) {
        QTextStream out(stdout);
        out << "CYBERSHOW_EVENT " << sanitize(name);
        if (!payload.isEmpty()) out << " " << sanitize(payload);
        out << Qt::endl;
    }

private:
    static QString sanitize(QString value) {
        value.replace('\n', ' ');
        value.replace('\r', ' ');
        return value.trimmed();
    }
};

} // namespace cybershow

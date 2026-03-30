#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include "InitScreen.h"
#include "MainWindow.h"

// Returns an error description, or an empty string if everything is valid.
static QString validateResources()
{
    // ── Embedded Qt resources (compiled into the binary) ─────────────────────
    // These should never be missing, but we validate them so a bad build is
    // caught immediately rather than silently misbehaving at runtime.

    for (const QString& r : { QStringLiteral(":/world_map.svg"), QStringLiteral(":/flying-cuarzito.png") }) {
        QFile f(r);
        if (!f.open(QIODevice::ReadOnly) || f.size() == 0)
            return QString("Embedded resource is missing or empty: %1").arg(r);
    }

    {
        QFile f(":/demo_events.json");
        if (!f.open(QIODevice::ReadOnly))
            return "Cannot open embedded resource: demo_events.json";
        QJsonParseError e;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &e);
        if (e.error != QJsonParseError::NoError)
            return QString("Corrupted embedded resource demo_events.json: %1").arg(e.errorString());
        if (!doc.isArray() || doc.array().isEmpty())
            return "demo_events.json must be a non-empty JSON array";
    }

    // ── Filesystem resources (read at runtime from resources/) ────────────────
    // These are under version control. Recover from GitHub if missing.

    auto findFile = [](const QString& name) -> QString {
        const QStringList candidates = {
            QDir::currentPath() + "/resources/" + name,
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../resources/" + name),
            QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../resources/" + name),
            QCoreApplication::applicationDirPath() + "/" + name,
        };
        for (const QString& c : candidates)
            if (QFile::exists(c)) return c;
        return {};
    };

    // regions.json
    {
        QString path = findFile("regions.json");
        if (path.isEmpty())
            return "Required file not found: resources/regions.json\n\nRecover it from the GitHub repository.";
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return "Cannot open: " + path;
        QJsonParseError e;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &e);
        if (e.error != QJsonParseError::NoError)
            return QString("Corrupted file regions.json: %1").arg(e.errorString());
        if (!doc.isObject() || doc.object()["regions"].toObject().isEmpty())
            return "regions.json is missing or has an empty 'regions' object";
    }

    // services.json
    {
        QString path = findFile("services.json");
        if (path.isEmpty())
            return "Required file not found: resources/services.json\n\nRecover it from the GitHub repository.";
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return "Cannot open: " + path;
        QJsonParseError e;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &e);
        if (e.error != QJsonParseError::NoError)
            return QString("Corrupted file services.json: %1").arg(e.errorString());
        if (!doc.isObject() || doc.object().isEmpty())
            return "services.json must be a non-empty JSON object";
    }

    return {};
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QString resourceError = validateResources();
    if (!resourceError.isEmpty()) {
        QMessageBox::critical(
            nullptr,
            "Public Wi-Fi Cybershow — Resource Error",
            "The application cannot start because a required resource file is missing or corrupted.\n\n"
            + resourceError
        );
        return 1;
    }

    app.setStyle("Fusion");
    app.setStyleSheet(
        "QMainWindow { background-color: #090C10; color: #FFFFFF; }"
        "QWidget { background-color: #090C10; color: #FFFFFF; }"
        "QPushButton {"
        "  background-color: #1E283C; border: 1px solid #32405A;"
        "  color: #FFFFFF; padding: 5px; border-radius: 4px;"
        "}"
        "QPushButton:hover { background-color: #283655; }"
        "QPushButton:pressed { background-color: #00FF44; color: #000000; }"
        "QListWidget { background-color: #202020; color: #FFFFFF; border: 1px solid #32405A; }"
        "QTextEdit { background-color: #202020; color: #00FF44; font-family: Consolas; border: 1px solid #32405A; }"
        "QLabel { color: #FFFFFF; }"
    );

    InitScreen initScreen;
    if (initScreen.exec() != QDialog::Accepted)
        return 0;

    MainWindow window(initScreen.config());
    window.showMaximized();

    return app.exec();
}

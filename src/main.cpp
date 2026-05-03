#include <QApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMessageBox>
#include <QScreen>
#include <QWidget>
#include "InitScreen.h"
#include "MainWindow.h"
#include "cybershow/common/CyberAppMode.h"
#include "cybershow/common/CyberOrchestratorProtocol.h"
#include "cybershow/common/CyberOperationalLog.h"
#include "cybershow/ui/CyberTheme.h"

static QString launchModeName(ShowConfig::LaunchMode mode)
{
    return mode == ShowConfig::LaunchMode::Configure
        ? QStringLiteral("configure")
        : QStringLiteral("show");
}

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

static ShowConfig configFromLaunchOptions(const cybershow::AppLaunchOptions& options)
{
    ShowConfig cfg;
    cfg.launchMode = (options.launchMode == cybershow::LaunchMode::Configure)
        ? ShowConfig::LaunchMode::Configure
        : ShowConfig::LaunchMode::Show;
    cfg.profile = options.profile.toLower();
    cfg.originalModeArgument = options.originalModeArgument;
    cfg.configPath = options.configPath;
    cfg.screenIndex = options.screenIndex;
    cfg.fullscreen = options.fullscreen;
    cfg.windowed = options.windowed;
    cfg.debug = options.debug;

    if (cfg.profile == "live") {
        cfg.mode = ShowConfig::Mode::Normal;
        cfg.profile = "live";
    } else {
        cfg.mode = ShowConfig::Mode::Demo;
        if (cfg.profile != "dev") {
            cfg.profile = "demo";
        }
    }

    cfg.actSequence = false;
    return cfg;
}

static void applyLaunchOptions(ShowConfig& cfg, const cybershow::AppLaunchOptions& options)
{
    const ShowConfig selectedMode = cfg;
    cfg = configFromLaunchOptions(options);

    if (selectedMode.mode == ShowConfig::Mode::Demo) {
        cfg.mode = ShowConfig::Mode::Demo;
        cfg.profile = "demo";
        cfg.actSequence = selectedMode.actSequence;
    } else {
        cfg.mode = ShowConfig::Mode::Normal;
        cfg.profile = "live";
        cfg.actSequence = false;
    }
}

static void placeWindowOnRequestedScreen(QWidget& window, int screenIndex)
{
    if (screenIndex < 0) {
        return;
    }

    const auto screens = QGuiApplication::screens();
    if (screenIndex >= screens.size()) {
        return;
    }

    const QRect available = screens.at(screenIndex)->availableGeometry();
    window.setGeometry(available);
    window.move(available.topLeft());
}

static void showMainWindow(MainWindow& window, const ShowConfig& config)
{
    placeWindowOnRequestedScreen(window, config.screenIndex);

    if (config.fullscreen) {
        window.showFullScreen();
    } else if (config.windowed) {
        if (config.screenIndex < 0) {
            window.resize(1280, 720);
        }
        window.show();
    } else {
        window.showMaximized();
    }
}

static bool runSetup(ShowConfig& config, const cybershow::AppLaunchOptions& options)
{
    InitScreen initScreen;
    initScreen.setConfig(config);
    if (initScreen.exec() != QDialog::Accepted) {
        return false;
    }

    config = initScreen.config();
    applyLaunchOptions(config, options);
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    cybershow::OperationalLog::configure(QStringLiteral("startup"), QStringLiteral("unknown"));
    cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("startup"), QStringLiteral("Application process started"));

    const cybershow::ParseResult launchParse =
        cybershow::parseAppLaunchOptions(QCoreApplication::arguments());
    if (!launchParse.ok) {
        cybershow::OrchestratorProtocol::status("ERROR", "INVALID_ARGUMENTS");
        cybershow::OperationalLog::write(QStringLiteral("ERROR"), QStringLiteral("startup"), QStringLiteral("Invalid launch arguments"));
        QMessageBox::critical(
            nullptr,
            "Public Wi-Fi Cybershow - Startup Error",
            "The application cannot start because the launch arguments are invalid.\n\n"
            + launchParse.error
        );
        return 2;
    }

    const QString resourceError = validateResources();
    if (!resourceError.isEmpty()) {
        cybershow::OrchestratorProtocol::status("ERROR", "RESOURCE_VALIDATION");
        cybershow::OperationalLog::write(QStringLiteral("ERROR"), QStringLiteral("startup"), QStringLiteral("Resource validation failed"));
        QMessageBox::critical(
            nullptr,
            "Public Wi-Fi Cybershow — Resource Error",
            "The application cannot start because a required resource file is missing or corrupted.\n\n"
            + resourceError
        );
        return 1;
    }

    app.setStyle("Fusion");
    app.setStyleSheet(CyberTheme::globalStyleSheet());
    cybershow::OrchestratorProtocol::status("READY");
    cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("startup"), QStringLiteral("Application ready"));

    bool shouldShowSetup = cybershow::setupAvailable(launchParse.options);
    ShowConfig config = (shouldShowSetup && !launchParse.options.profileProvided)
        ? ShowConfig{}
        : configFromLaunchOptions(launchParse.options);

    while (true) {
        if (shouldShowSetup && !runSetup(config, launchParse.options)) {
            cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("setup"), QStringLiteral("Setup cancelled by operator"));
            return 0;
        }

        cybershow::OperationalLog::configure(launchModeName(config.launchMode), config.profile);
        cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("runtime"), QStringLiteral("Creating runtime window"));

        bool setupRequested = false;
        MainWindow window(config);
        QObject::connect(&window, &MainWindow::setupRequested, &app, [&]() {
            setupRequested = true;
            cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("runtime"), QStringLiteral("Setup requested from runtime"));
            window.close();
            app.quit();
        });

        showMainWindow(window, config);
        cybershow::OrchestratorProtocol::status("RUNNING");
        cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("runtime"), QStringLiteral("Runtime window shown"));

        const int result = app.exec();
        if (!setupRequested) {
            cybershow::OperationalLog::write(QStringLiteral("INFO"), QStringLiteral("runtime"), QString("Application exited with code %1").arg(result));
            return result;
        }

        shouldShowSetup = true;
    }
}

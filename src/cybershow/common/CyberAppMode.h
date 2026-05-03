#pragma once

#include <QString>
#include <QStringList>
#include <optional>

namespace cybershow {

enum class LaunchMode {
    Configure,
    Demo,
    Live
};

struct AppLaunchOptions {
    LaunchMode launchMode = LaunchMode::Configure;
    bool fullscreen = false;
    bool windowed = false;
    int screenIndex = -1;
    QString configPath;
    QString profile = "demo";
    bool profileProvided = false;
    bool debug = false;
    QString originalModeArgument = "--configure";
};

struct ParseResult {
    bool ok = true;
    AppLaunchOptions options;
    QString error;
};

ParseResult parseAppLaunchOptions(const QStringList& arguments);
QString launchModeToString(LaunchMode mode);
bool setupAvailable(const AppLaunchOptions& options);

} // namespace cybershow

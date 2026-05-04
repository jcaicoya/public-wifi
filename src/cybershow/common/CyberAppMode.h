#pragma once

#include <QString>
#include <QStringList>
#include <optional>

namespace cybershow {

enum class LaunchMode {
    Demo,
    Live
};

struct AppLaunchOptions {
    LaunchMode launchMode = LaunchMode::Live;
    bool fullscreen = false;
    bool windowed = false;
    int screenIndex = -1;
    QString configPath;
    QString profile = "live";
    bool profileProvided = false;
    bool debug = false;
    QString originalModeArgument = "--live";
};

struct ParseResult {
    bool ok = true;
    AppLaunchOptions options;
    QString error;
};

ParseResult parseAppLaunchOptions(const QStringList& arguments);
QString launchModeToString(LaunchMode mode);

} // namespace cybershow

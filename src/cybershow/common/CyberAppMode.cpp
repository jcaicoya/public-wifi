#include "CyberAppMode.h"

namespace cybershow {

QString launchModeToString(LaunchMode mode) {
    switch (mode) {
    case LaunchMode::Demo: return "demo";
    case LaunchMode::Live: return "live";
    }
    return "unknown";
}

ParseResult parseAppLaunchOptions(const QStringList& arguments) {
    ParseResult result;
    bool modeSeen = false;

    // arguments normally includes argv[0]. Start at 1.
    for (int i = 1; i < arguments.size(); ++i) {
        const QString arg = arguments.at(i);

        if (arg == "--configure") {
            result.ok = false;
            result.error = "Configure mode has been removed. Launch with --demo or --live.";
            return result;
        }

        if (arg == "--demo" || arg == "--live") {
            if (modeSeen) {
                result.ok = false;
                result.error = "Multiple launch modes provided. Use only one of --demo or --live.";
                return result;
            }
            modeSeen = true;
            result.options.originalModeArgument = arg;
            if (arg == "--demo") {
                result.options.launchMode = LaunchMode::Demo;
            } else {
                result.options.launchMode = LaunchMode::Live;
            }
        } else if (arg == "--fullscreen") {
            result.options.fullscreen = true;
        } else if (arg == "--windowed") {
            result.options.windowed = true;
        } else if (arg == "--debug") {
            result.options.debug = true;
        } else if (arg == "--screen") {
            if (i + 1 >= arguments.size()) {
                result.ok = false;
                result.error = "Missing value after --screen.";
                return result;
            }
            bool ok = false;
            const int value = arguments.at(++i).toInt(&ok);
            if (!ok || value < 0) {
                result.ok = false;
                result.error = "Invalid --screen value. Expected a non-negative integer.";
                return result;
            }
            result.options.screenIndex = value;
        } else if (arg == "--config") {
            if (i + 1 >= arguments.size()) {
                result.ok = false;
                result.error = "Missing value after --config.";
                return result;
            }
            result.options.configPath = arguments.at(++i);
        } else if (arg == "--profile") {
            if (i + 1 >= arguments.size()) {
                result.ok = false;
                result.error = "Missing value after --profile.";
                return result;
            }
            result.options.profileProvided = true;
            result.options.profile = arguments.at(++i).toLower();
            if (result.options.profile != "demo"
                && result.options.profile != "live"
                && result.options.profile != "dev") {
                result.ok = false;
                result.error = QString("Invalid --profile value: %1. Expected demo, live or dev.")
                                   .arg(result.options.profile);
                return result;
            }
        } else {
            result.ok = false;
            result.error = QString("Unknown argument: %1").arg(arg);
            return result;
        }
    }

    if (result.options.fullscreen && result.options.windowed) {
        result.ok = false;
        result.error = "Use only one of --fullscreen or --windowed.";
        return result;
    }

    return result;
}

} // namespace cybershow

#pragma once

#include <QString>

struct ShowConfig
{
    enum class LaunchMode { Configure, Demo, Live };
    enum class Mode { Normal, Demo };

    LaunchMode launchMode = LaunchMode::Configure;
    Mode mode        = Mode::Normal;

    QString profile = "live";
    QString originalModeArgument = "--configure";
    QString configPath;
    int screenIndex = -1;
    bool fullscreen = false;
    bool windowed = false;
    bool debug = false;
};

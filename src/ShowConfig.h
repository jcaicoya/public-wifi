#pragma once

#include <QString>

struct ShowConfig
{
    enum class LaunchMode { Configure, Show };
    enum class Mode { Normal, Demo };

    LaunchMode launchMode = LaunchMode::Configure;
    Mode mode        = Mode::Normal;
    bool actSequence = false;   // Demo only: cycle screens unattended

    QString profile = "live";
    QString originalModeArgument = "--configure";
    QString configPath;
    int screenIndex = -1;
    bool fullscreen = false;
    bool windowed = false;
    bool debug = false;
};

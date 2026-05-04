#pragma once

#include <QString>

struct ShowConfig
{
    enum class LaunchMode { Demo, Live };
    enum class Mode { Normal, Demo };

    LaunchMode launchMode = LaunchMode::Live;
    Mode mode        = Mode::Normal;

    QString profile = "live";
    QString originalModeArgument = "--live";
    QString configPath;
    int screenIndex = -1;
    bool fullscreen = false;
    bool windowed = false;
    bool debug = false;
};

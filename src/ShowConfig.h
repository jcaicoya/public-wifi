#pragma once

struct ShowConfig
{
    enum class Mode { Normal, Demo };

    Mode mode        = Mode::Normal;
    bool actSequence = false;   // Demo only: cycle screens unattended
};

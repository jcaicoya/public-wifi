#include "AlertBanner.h"

#include "CyberTheme.h"

#include <QHBoxLayout>
#include <QLabel>

AlertBanner::AlertBanner(QWidget* parent) : QFrame(parent) {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 14, 20, 14);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("font-size: 20px; font-weight: 900; letter-spacing: 1px;");
    layout->addWidget(m_label);

    setSeverity(Severity::Info);
}

void AlertBanner::setMessage(const QString& message) {
    m_label->setText(message);
}

void AlertBanner::setSeverity(Severity severity) {
    QString color;
    QString background;

    switch (severity) {
    case Severity::Info:
        color = CyberTheme::AccentCyan;
        background = "rgba(0, 209, 255, 0.08)";
        break;
    case Severity::Success:
        color = CyberTheme::AccentGreen;
        background = "rgba(0, 255, 85, 0.08)";
        break;
    case Severity::Warning:
        color = CyberTheme::Warning;
        background = "rgba(255, 176, 0, 0.10)";
        break;
    case Severity::Critical:
        color = CyberTheme::Error;
        background = "rgba(255, 51, 71, 0.12)";
        break;
    }

    setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border: 2px solid %2; border-radius: 10px; }"
        "QLabel { color: %2; }"
    ).arg(background, color));
}

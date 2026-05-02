#include "MetricCard.h"

#include "CyberTheme.h"

#include <QLabel>
#include <QVBoxLayout>

MetricCard::MetricCard(QWidget* parent) : QFrame(parent), m_accent(CyberTheme::color(CyberTheme::AccentPrimary)) {
    setObjectName("CyberPanelRaised");
    setMinimumHeight(160);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(10);

    m_label = new QLabel(this);
    m_label->setObjectName("MetricLabel");
    m_label->setAlignment(Qt::AlignCenter);

    m_value = new QLabel(this);
    m_value->setObjectName("MetricValue");
    m_value->setAlignment(Qt::AlignCenter);

    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName("MutedLabel");
    m_subtitle->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_label);
    layout->addStretch(1);
    layout->addWidget(m_value);
    layout->addStretch(1);
    layout->addWidget(m_subtitle);

    setAccentColor(m_accent);
}

void MetricCard::setLabel(const QString& label) {
    m_label->setText(label);
}

void MetricCard::setValue(const QString& value) {
    m_value->setText(value);
}

void MetricCard::setSubtitle(const QString& subtitle) {
    m_subtitle->setText(subtitle);
}

void MetricCard::setAccentColor(const QColor& color) {
    m_accent = color;
    const QString css = QStringLiteral(
        "QFrame#CyberPanelRaised { background-color: #151922; border: 1px solid %1; border-radius: 14px; }"
        "QLabel#MetricValue { color: %1; }"
    ).arg(color.name());
    setStyleSheet(css);
}

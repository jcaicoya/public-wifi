#include "InitScreen.h"
#include "cybershow/ui/CyberBackgroundWidget.h"

#include <QCheckBox>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace
{
bool focusIsEditable(QWidget* focusWidget)
{
    if (!focusWidget) return false;
    if (qobject_cast<QLineEdit*>(focusWidget)) return true;
    if (qobject_cast<QTextEdit*>(focusWidget)) return true;
    if (qobject_cast<QPlainTextEdit*>(focusWidget)) return true;
    if (qobject_cast<QAbstractSpinBox*>(focusWidget)) return true;
    if (auto* combo = qobject_cast<QComboBox*>(focusWidget)) return combo->isEditable();
    return false;
}
}

InitScreen::InitScreen(QWidget* parent)
    : QDialog(parent, Qt::Window)
{
    setWindowTitle("Public Wi-Fi - Configuracion tecnica");
    setWindowState(Qt::WindowMaximized);
    buildUi();
}

void InitScreen::buildUi()
{
    auto* background = new CyberBackgroundWidget(this);
    background->setGlowIntensity(0.55);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(background);

    auto* mainLayout = new QVBoxLayout(background);
    mainLayout->setContentsMargins(48, 48, 48, 48);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto* configBox = new QFrame(background);
    configBox->setObjectName("CyberPanelRaised");
    configBox->setFixedWidth(560);

    auto* configLayout = new QVBoxLayout(configBox);
    configLayout->setContentsMargins(34, 30, 34, 34);
    configLayout->setSpacing(18);

    auto* kicker = new QLabel("PUBLIC WI-FI", configBox);
    kicker->setObjectName("KickerLabel");
    kicker->setAlignment(Qt::AlignCenter);
    configLayout->addWidget(kicker);

    auto* title = new QLabel("Public Wi-Fi - Configuracion tecnica", configBox);
    title->setObjectName("ScreenTitle");
    title->setAlignment(Qt::AlignCenter);
    configLayout->addWidget(title);

    auto* subtitle = new QLabel(
        "Preparacion del modulo de red publica para ensayo o show.",
        configBox);
    subtitle->setObjectName("MutedLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    configLayout->addWidget(subtitle);

    auto* runModeLabel = new QLabel("Modo de funcionamiento", configBox);
    runModeLabel->setObjectName("FieldLabel");
    configLayout->addWidget(runModeLabel);

    m_runModeCombo = new QComboBox(configBox);
    m_runModeCombo->setFocusPolicy(Qt::NoFocus);
    m_runModeCombo->setMinimumHeight(46);
    m_runModeCombo->addItem("Live - router GL.iNet");
    m_runModeCombo->addItem("Demo - eventos simulados");
    configLayout->addWidget(m_runModeCombo);

    m_statusLabel = new QLabel(configBox);
    m_statusLabel->setObjectName("StatusInfo");
    m_statusLabel->setWordWrap(true);
    configLayout->addWidget(m_statusLabel);

    m_demoGroup = new QGroupBox("Control demo", configBox);
    m_demoGroup->setStyleSheet(
        "QGroupBox { color: #F2F5F8; font-weight: 800; border: 1px solid #293241;"
        "            border-radius: 8px; margin-top: 12px; padding-top: 14px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
    );
    m_demoGroup->setVisible(false);
    auto* demoLayout = new QVBoxLayout(m_demoGroup);
    demoLayout->setSpacing(12);

    m_actSeqCheck = new QCheckBox("Secuencia automatica", m_demoGroup);
    demoLayout->addWidget(m_actSeqCheck);

    auto* actSeqDesc = new QLabel("Recorre las pantallas sin operador y mantiene el objetivo demo.", m_demoGroup);
    actSeqDesc->setObjectName("MutedLabel");
    actSeqDesc->setWordWrap(true);
    demoLayout->addWidget(actSeqDesc);

    configLayout->addWidget(m_demoGroup);

    auto* startBtn = new QPushButton("INICIAR SHOW", configBox);
    startBtn->setObjectName("PrimaryButton");
    startBtn->setFocusPolicy(Qt::NoFocus);
    startBtn->setMinimumHeight(52);
    configLayout->addWidget(startBtn);

    mainLayout->addWidget(configBox, 0, Qt::AlignCenter);

    connect(m_runModeCombo, &QComboBox::currentIndexChanged, this, &InitScreen::onModeChanged);
    connect(startBtn, &QPushButton::clicked, this, &QDialog::accept);

    onModeChanged(); // set initial state
}

void InitScreen::setConfig(const ShowConfig& config)
{
    const bool isDemo = config.mode == ShowConfig::Mode::Demo;
    m_runModeCombo->setCurrentIndex(isDemo ? 1 : 0);
    m_actSeqCheck->setChecked(isDemo && config.actSequence);
    onModeChanged();
}

void InitScreen::keyPressEvent(QKeyEvent* event)
{
    if (!event) {
        return;
    }

    if (!focusIsEditable(focusWidget())) {
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
        case Qt::Key_Right:
        case Qt::Key_1:
            accept();
            return;
        case Qt::Key_Escape:
            event->accept();
            return;
        default:
            break;
        }
    }

    QDialog::keyPressEvent(event);
}

void InitScreen::onModeChanged()
{
    const bool isDemo = (m_runModeCombo->currentIndex() == 1);
    m_demoGroup->setVisible(isDemo);
    m_statusLabel->setText(isDemo
        ? "Modo DEMO: no requiere router. Los eventos se inyectan desde demo_events.json."
        : "Modo LIVE: conecta con GL-MT300N-V2 en 192.168.8.1. El router debe estar accesible."
    );
}

ShowConfig InitScreen::config() const
{
    ShowConfig cfg;
    cfg.mode        = (m_runModeCombo->currentIndex() == 1) ? ShowConfig::Mode::Demo : ShowConfig::Mode::Normal;
    cfg.actSequence = (cfg.mode == ShowConfig::Mode::Demo) && m_actSeqCheck->isChecked();
    return cfg;
}

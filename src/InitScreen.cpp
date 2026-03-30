#include "InitScreen.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

InitScreen::InitScreen(QWidget* parent)
    : QDialog(parent, Qt::Window)
{
    setWindowTitle("Public Wi-Fi Cybershow");
    setWindowState(Qt::WindowMaximized);
    setStyleSheet("background-color: black; color: white; font-family: 'Segoe UI', sans-serif;");
    buildUi();
}

void InitScreen::buildUi()
{
    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(40, 40, 40, 20);
    mainLayout->setSpacing(20);

    // ── Two-column body, centred on screen ───────────────────────────────────
    auto* body = new QHBoxLayout();
    body->setSpacing(40);
    body->setAlignment(Qt::AlignCenter);
    body->addStretch(1);

    // ── LEFT COLUMN ──────────────────────────────────────────────────────────
    auto* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(18);
    leftColumn->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto* title = new QLabel("TECHNICAL SETUP");
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: #888;");
    title->setAlignment(Qt::AlignCenter);
    leftColumn->addWidget(title);

    // Config box (card)
    auto* configBox = new QFrame();
    configBox->setStyleSheet("background: #111; border: 2px solid #333; border-radius: 15px;");
    configBox->setFixedWidth(460);
    auto* configLayout = new QVBoxLayout(configBox);
    configLayout->setContentsMargins(30, 30, 30, 30);
    configLayout->setSpacing(18);

    const QString comboStyle =
        "QComboBox { background: #222; color: white; border: 1px solid #444; border-radius: 10px;"
        "            padding: 10px 36px 10px 10px; font-size: 16px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right;"
        "                       width: 30px; border: none; background: transparent; }"
        "QComboBox::down-arrow { width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background: #222; color: white; border: 1px solid #444;"
        "                              selection-background-color: #0f5f33; }";

    // Run mode
    auto* runModeLabel = new QLabel("Run mode:");
    runModeLabel->setStyleSheet("border: none; font-size: 16px;");
    configLayout->addWidget(runModeLabel);

    m_runModeCombo = new QComboBox();
    m_runModeCombo->setFocusPolicy(Qt::NoFocus);
    m_runModeCombo->setStyleSheet(comboStyle);
    m_runModeCombo->setMinimumHeight(46);
    m_runModeCombo->addItem("Normal mode");
    m_runModeCombo->addItem("Demo mode");
    configLayout->addWidget(m_runModeCombo);

    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("border: none; font-size: 14px; color: #9ad39a;");
    configLayout->addWidget(m_statusLabel);

    // Demo settings group
    m_demoGroup = new QGroupBox("Demo control");
    m_demoGroup->setStyleSheet(
        "QGroupBox { font-size: 15px; font-weight: bold; border: 1px solid #333;"
        "            border-radius: 10px; margin-top: 10px; padding-top: 10px; }"
    );
    m_demoGroup->setVisible(false);
    auto* demoLayout = new QVBoxLayout(m_demoGroup);
    demoLayout->setSpacing(12);

    m_actSeqCheck = new QCheckBox("Act sequence");
    m_actSeqCheck->setStyleSheet(
        "QCheckBox { color: white; font-size: 16px; spacing: 12px; }"
        "QCheckBox::indicator { width: 22px; height: 22px; border: 1px solid #4c6a88;"
        "                       border-radius: 6px; background: #182634; }"
        "QCheckBox::indicator:checked { background: #1f4e79; border: 1px solid #7fb1e3; }"
    );
    demoLayout->addWidget(m_actSeqCheck);

    auto* actSeqDesc = new QLabel("Cycles all screens unattended.\nAlways targets the same device.");
    actSeqDesc->setStyleSheet("border: none; font-size: 13px; color: #888;");
    demoLayout->addWidget(actSeqDesc);

    configLayout->addWidget(m_demoGroup);

    // START SHOW
    auto* startBtn = new QPushButton("START SHOW");
    startBtn->setFocusPolicy(Qt::NoFocus);
    startBtn->setStyleSheet(
        "QPushButton { background: #0078d4; color: white; border: none;"
        "              padding: 15px; font-size: 18px; font-weight: bold; margin-top: 10px; }"
        "QPushButton:hover { background: #005fa3; }"
        "QPushButton:pressed { background: #004080; }"
    );
    configLayout->addWidget(startBtn);

    leftColumn->addWidget(configBox);
    body->addLayout(leftColumn);

    // ── RIGHT COLUMN — mascot ────────────────────────────────────────────────
    auto* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(22);
    rightColumn->setAlignment(Qt::AlignCenter);

    auto* logo = new QLabel();
    logo->setPixmap(QPixmap(":/flying-cuarzito.png").scaled(360, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);
    rightColumn->addWidget(logo, 0, Qt::AlignHCenter);

    body->addLayout(rightColumn);
    body->addStretch(1);

    mainLayout->addStretch(1);
    mainLayout->addLayout(body);
    mainLayout->addStretch(1);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_runModeCombo, &QComboBox::currentIndexChanged, this, &InitScreen::onModeChanged);
    connect(startBtn, &QPushButton::clicked, this, &QDialog::accept);

    onModeChanged(); // set initial state
}

void InitScreen::onModeChanged()
{
    const bool isDemo = (m_runModeCombo->currentIndex() == 1);
    m_demoGroup->setVisible(isDemo);
    m_statusLabel->setText(isDemo
        ? "No router required. Events are injected from demo_events.json."
        : "Connects to GL-MT300N-V2 at 192.168.8.1. Router must be reachable."
    );
}

ShowConfig InitScreen::config() const
{
    ShowConfig cfg;
    cfg.mode        = (m_runModeCombo->currentIndex() == 1) ? ShowConfig::Mode::Demo : ShowConfig::Mode::Normal;
    cfg.actSequence = (cfg.mode == ShowConfig::Mode::Demo) && m_actSeqCheck->isChecked();
    return cfg;
}

#include "PolygonEditor.h"
#include "MapCanvas.h"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

static const QString kDark   = "#090C10";
static const QString kPanel  = "#0d1117";
static const QString kBorder = "#1a4a5a";
static const QString kCyan   = "#00FFFF";
static const QString kGreen  = "#00FF44";
static const QString kDim    = "#445566";

// Returns the canonical save/load path, trying working dir first, then relative to exe.
QString PolygonEditor::resolveSavePath() const
{
    // 1. Working directory (CLion run config: set to project root)
    QString cwdPath = QDir::currentPath() + "/resources/regions.json";
    if (QDir(QDir::currentPath() + "/resources").exists())
        return QDir::cleanPath(cwdPath);

    // 2. Navigate up from exe until we find a resources/ sibling
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (d.exists("resources"))
            return QDir::cleanPath(d.filePath("resources/regions.json"));
        d.cdUp();
    }

    // 3. Fallback: next to the executable
    return QCoreApplication::applicationDirPath() + "/regions.json";
}

PolygonEditor::PolygonEditor(QWidget* parent) : QMainWindow(parent)
{
    setStyleSheet(QString("QMainWindow, QWidget { background:%1; color:%2; }")
                  .arg(kDark).arg(kCyan));
    buildUi();

    // Auto-load existing regions.json if present
    const QString path = resolveSavePath();
    if (QFile::exists(path)) {
        if (m_canvas->loadFromJson(path)) {
            // Rebuild the region list to reflect what was loaded
            m_regionList->clear();
            for (const QString& name : m_canvas->regionNames())
                m_regionList->addItem(name);
            m_regionList->setCurrentRow(0);
            m_canvas->setActiveRegion(m_canvas->regionNames().first());
            statusBar()->showMessage("  Loaded: " + path, 4000);
        }
    }
    m_pathLabel->setText("  Save path: " + resolveSavePath());
}

void PolygonEditor::buildUi()
{
    // Canvas is created first — it owns the region list data
    m_canvas = new MapCanvas;

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(5);

    // ── Top area: region list | canvas ────────────────────────────────────────
    auto* hSplit = new QSplitter(Qt::Horizontal);
    hSplit->setHandleWidth(4);
    hSplit->setStyleSheet("QSplitter::handle { background:#1a4a5a; }");

    // Region list
    m_regionList = new QListWidget;
    m_regionList->setFixedWidth(170);
    m_regionList->setStyleSheet(
        QString("QListWidget {"
                "  background:%1; color:%2;"
                "  border:1px solid %3;"
                "  font-family:Consolas; font-size:11px; padding:2px;"
                "}"
                "QListWidget::item { padding:5px 8px; }"
                "QListWidget::item:selected { background:#0d2e3a; color:%4; }"
                "QListWidget::item:hover:!selected { background:#101a22; }")
        .arg(kPanel).arg(kCyan).arg(kBorder).arg(kCyan)
    );
    for (const QString& name : m_canvas->regionNames())
        m_regionList->addItem(name);
    m_regionList->setCurrentRow(0);

    auto* listLabel = new QLabel("REGIONS");
    listLabel->setStyleSheet(QString(
        "color:%1; font-family:Consolas; font-size:10px; font-weight:bold;"
        "padding:4px 8px; background:%2; border-bottom:1px solid %3;")
        .arg(kDim).arg(kPanel).arg(kBorder));
    auto* listPane = new QWidget;
    auto* listLayout = new QVBoxLayout(listPane);
    listLayout->setContentsMargins(0,0,0,0);
    listLayout->setSpacing(0);
    listLayout->addWidget(listLabel);
    listLayout->addWidget(m_regionList);

    hSplit->addWidget(listPane);
    hSplit->addWidget(m_canvas);
    hSplit->setStretchFactor(0, 0);
    hSplit->setStretchFactor(1, 1);

    root->addWidget(hSplit, 1);

    // ── Bottom area: code output + buttons ────────────────────────────────────
    auto* codeLabel = new QLabel("C++ OUTPUT — ready to paste into MapView::buildMapRegions()");
    codeLabel->setStyleSheet(QString(
        "color:%1; font-family:Consolas; font-size:10px; font-weight:bold;"
        "padding:4px 8px; background:%2; border:1px solid %3; border-bottom:none;")
        .arg(kDim).arg(kPanel).arg(kBorder));
    root->addWidget(codeLabel);

    m_codeOutput = new QTextEdit;
    m_codeOutput->setReadOnly(true);
    m_codeOutput->setFixedHeight(160);
    m_codeOutput->setFont(QFont("Consolas", 9));
    m_codeOutput->setStyleSheet(QString(
        "QTextEdit { background:%1; color:#c9d1d9; border:1px solid %2; }")
        .arg(kPanel).arg(kBorder));
    root->addWidget(m_codeOutput);

    // Button row
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);

    auto makeBtn = [&](const QString& label) {
        auto* btn = new QPushButton(label);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background:#0d2a35; color:%1; border:1px solid %2;"
            "  padding:5px 16px; font-family:Consolas; font-size:11px; font-weight:bold;"
            "}"
            "QPushButton:hover { background:#0d3a45; }"
            "QPushButton:pressed { background:#0a2030; }")
            .arg(kCyan).arg(kBorder));
        return btn;
    };

    auto* btnRegion = makeBtn("Copy Region");
    auto* btnAll    = makeBtn("Copy All Regions");
    auto* btnSave   = makeBtn("💾  Save regions.json");
    btnSave->setStyleSheet(btnSave->styleSheet() +
        "QPushButton { color:#00FF44; border-color:#00aa33; }");

    auto* hint = new QLabel(
        "Left-click empty space → insert node on nearest edge   |   "
        "Drag node → move   |   Right-click node → delete");
    hint->setStyleSheet(QString(
        "color:%1; font-family:Consolas; font-size:10px;").arg(kDim));

    btnRow->addWidget(btnSave);
    btnRow->addWidget(btnRegion);
    btnRow->addWidget(btnAll);
    btnRow->addStretch();
    btnRow->addWidget(hint);
    root->addLayout(btnRow);

    // Path label
    m_pathLabel = new QLabel;
    m_pathLabel->setStyleSheet(QString(
        "color:%1; font-family:Consolas; font-size:9px; padding:2px 4px;").arg(kDim));
    root->addWidget(m_pathLabel);

    // ── Status bar ────────────────────────────────────────────────────────────
    statusBar()->setStyleSheet(QString(
        "QStatusBar { background:%1; color:%2; font-family:Consolas; font-size:10px; }")
        .arg(kPanel).arg(kDim));

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_regionList, &QListWidget::currentRowChanged,
            this, &PolygonEditor::onRegionSelected);

    connect(m_canvas, &MapCanvas::polygonChanged,
            this, &PolygonEditor::onPolygonChanged);

    connect(m_canvas, &MapCanvas::statusChanged,
            this, [this](const QString& msg) { statusBar()->showMessage(msg); });

    connect(btnRegion, &QPushButton::clicked, this, &PolygonEditor::copyRegion);
    connect(btnAll,    &QPushButton::clicked, this, &PolygonEditor::copyAll);
    connect(btnSave,   &QPushButton::clicked, this, &PolygonEditor::saveRegions);

    // Seed the code box with the first region
    onPolygonChanged();
}

void PolygonEditor::onRegionSelected(int row)
{
    if (row < 0 || !m_canvas) return;
    m_canvas->setActiveRegion(m_canvas->regionNames().at(row));
    onPolygonChanged();
}

void PolygonEditor::onPolygonChanged()
{
    if (!m_canvas) return;
    int row = m_regionList->currentRow();
    if (row < 0) return;
    m_codeOutput->setPlainText(
        m_canvas->generateCode(m_canvas->regionNames().at(row)));
}

void PolygonEditor::copyRegion()
{
    QApplication::clipboard()->setText(m_codeOutput->toPlainText());
    statusBar()->showMessage("  Region code copied to clipboard.", 2500);
}

void PolygonEditor::copyAll()
{
    if (!m_canvas) return;
    const QString all = m_canvas->generateAllCode();
    m_codeOutput->setPlainText(all);
    QApplication::clipboard()->setText(all);
    statusBar()->showMessage("  All regions copied to clipboard.", 2500);
}

void PolygonEditor::saveRegions()
{
    if (!m_canvas) return;
    const QString path = resolveSavePath();
    if (m_canvas->saveToJson(path)) {
        statusBar()->showMessage("  Saved → " + path, 4000);
        m_pathLabel->setText("  Saved: " + path);
    } else {
        statusBar()->showMessage("  ERROR: could not write to " + path, 5000);
        m_pathLabel->setText("  ERROR saving to: " + path);
    }
}

#include "ServiceMapper.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>

static const QString kDark   = "#090C10";
static const QString kPanel  = "#0d1117";
static const QString kBorder = "#1a4a5a";
static const QString kCyan   = "#00FFFF";
static const QString kGreen  = "#00FF44";
static const QString kDim    = "#445566";

ServiceMapper::ServiceMapper(QWidget* parent) : QMainWindow(parent)
{
    setStyleSheet(QString("QMainWindow, QWidget { background:%1; color:%2; font-family:Consolas; }")
                  .arg(kDark).arg(kCyan));
    buildUi();
    loadData();
}

QString ServiceMapper::resolveRegionsPath() const
{
    QString cwdPath = QDir::currentPath() + "/resources/regions.json";
    if (QFile::exists(cwdPath)) return QDir::cleanPath(cwdPath);
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (d.exists("resources/regions.json")) return QDir::cleanPath(d.filePath("resources/regions.json"));
        d.cdUp();
    }
    return QCoreApplication::applicationDirPath() + "/regions.json";
}

QString ServiceMapper::resolveServicesPath() const
{
    QString cwdPath = QDir::currentPath() + "/resources/services.json";
    if (QDir(QDir::currentPath() + "/resources").exists()) return QDir::cleanPath(cwdPath);
    QDir d(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        if (d.exists("resources")) return QDir::cleanPath(d.filePath("resources/services.json"));
        d.cdUp();
    }
    return QCoreApplication::applicationDirPath() + "/services.json";
}

void ServiceMapper::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(15, 15, 15, 15);
    root->setSpacing(10);

    auto* title = new QLabel("SERVICE TO REGION MAPPING");
    title->setStyleSheet("font-size: 16px; font-weight: bold; color: #00FF44; margin-bottom: 5px;");
    root->addWidget(title);

    auto* mainLayout = new QHBoxLayout();
    mainLayout->setSpacing(20);

    // ── Left Side: Services List ─────────────────────────────────────────────
    auto* leftBox = new QGroupBox("Services");
    leftBox->setStyleSheet(QString("QGroupBox { border: 1px solid %1; margin-top: 10px; color: %2; } "
                                   "QGroupBox::title { subcontrol-origin: margin; left: 10px; }")
                           .arg(kBorder).arg(kDim));
    auto* leftLayout = new QVBoxLayout(leftBox);
    
    m_serviceList = new QListWidget();
    m_serviceList->setStyleSheet(QString("QListWidget { background:%1; border: none; font-size: 12px; }"
                                         "QListWidget::item { padding: 8px; border-bottom: 1px solid #1a2a35; }"
                                         "QListWidget::item:selected { background: #1a4a5a; color: %2; }")
                                 .arg(kPanel).arg(kCyan));
    leftLayout->addWidget(m_serviceList);

    auto* svcBtns = new QHBoxLayout();
    auto makeBtn = [&](const QString& txt, const QString& color = kCyan) {
        auto* b = new QPushButton(txt);
        b->setStyleSheet(QString("QPushButton { background: #0d2a35; border: 1px solid %1; color: %2; padding: 5px 10px; font-weight: bold; }"
                                 "QPushButton:hover { background: #1a4a5a; }")
                         .arg(kBorder).arg(color));
        return b;
    };
    auto* btnAddSvc = makeBtn("Add", kGreen);
    auto* btnRenSvc = makeBtn("Rename");
    auto* btnDelSvc = makeBtn("Delete", "#FF3333");
    svcBtns->addWidget(btnAddSvc);
    svcBtns->addWidget(btnRenSvc);
    svcBtns->addWidget(btnDelSvc);
    leftLayout->addLayout(svcBtns);

    mainLayout->addWidget(leftBox, 1);

    // ── Right Side: Details / Region Selection ──────────────────────────────
    auto* rightBox = new QGroupBox("Mapping Details");
    rightBox->setStyleSheet(leftBox->styleSheet());
    auto* rightLayout = new QVBoxLayout(rightBox);
    rightLayout->setSpacing(20);

    auto* label = new QLabel("Select region for selected service:");
    label->setStyleSheet(QString("color: %1;").arg(kDim));
    rightLayout->addWidget(label);

    m_regionCombo = new QComboBox();
    m_regionCombo->setStyleSheet(QString("QComboBox { background: %1; border: 1px solid %2; color: %3; padding: 8px; font-size: 13px; }"
                                         "QComboBox QAbstractItemView { background: %1; color: %3; selection-background-color: %2; }")
                                 .arg(kPanel).arg(kBorder).arg(kCyan));
    rightLayout->addWidget(m_regionCombo);
    rightLayout->addStretch();

    auto* btnSave = makeBtn("💾  SAVE services.json", kGreen);
    btnSave->setFixedHeight(45);
    rightLayout->addWidget(btnSave);

    mainLayout->addWidget(rightBox, 1);
    root->addLayout(mainLayout);

    m_pathLabel = new QLabel();
    m_pathLabel->setStyleSheet(QString("color: %1; font-size: 9px;").arg(kDim));
    root->addWidget(m_pathLabel);

    // ── Connections ──────────────────────────────────────────────────────────
    connect(m_serviceList, &QListWidget::currentRowChanged, this, &ServiceMapper::onServiceSelected);
    connect(m_regionCombo, &QComboBox::currentIndexChanged, this, &ServiceMapper::onRegionChanged);
    connect(btnAddSvc, &QPushButton::clicked, this, &ServiceMapper::addServiceDialog);
    connect(btnRenSvc, &QPushButton::clicked, this, &ServiceMapper::renameServiceDialog);
    connect(btnDelSvc, &QPushButton::clicked, this, &ServiceMapper::deleteService);
    connect(btnSave, &QPushButton::clicked, this, &ServiceMapper::saveServices);
}

void ServiceMapper::loadData()
{
    // 1. Load regions from regions.json
    QString regPath = resolveRegionsPath();
    QFile fReg(regPath);
    if (fReg.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(fReg.readAll());
        m_regionNames = doc.object()["region_order"].toVariant().toStringList();
        if (m_regionNames.isEmpty()) {
            // Fallback if region_order is missing
            m_regionNames = doc.object()["regions"].toObject().keys();
        }
        m_regionCombo->clear();
        m_regionCombo->addItems(m_regionNames);
    }

    // 2. Load existing mapping from services.json
    QString svcPath = resolveServicesPath();
    m_pathLabel->setText("  Save path: " + svcPath);
    QFile fSvc(svcPath);
    if (fSvc.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(fSvc.readAll());
        QJsonObject obj = doc.object();
        m_mapping.clear();
        for (const QString& s : obj.keys()) {
            m_mapping[s] = obj[s].toString();
        }
    }

    // Update list
    m_serviceList->clear();
    QStringList sorted = m_mapping.keys();
    sorted.sort(Qt::CaseInsensitive);
    for (const QString& s : sorted) {
        m_serviceList->addItem(s);
    }

    if (m_serviceList->count() > 0) m_serviceList->setCurrentRow(0);
}

void ServiceMapper::onServiceSelected(int row)
{
    if (row < 0) {
        m_regionCombo->setEnabled(false);
        return;
    }
    m_regionCombo->setEnabled(true);
    QString svc = m_serviceList->item(row)->text();
    QString reg = m_mapping.value(svc);
    int idx = m_regionNames.indexOf(reg);
    m_regionCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void ServiceMapper::onRegionChanged(int index)
{
    if (index < 0 || !m_serviceList->currentItem()) return;
    QString svc = m_serviceList->currentItem()->text();
    m_mapping[svc] = m_regionNames.at(index);
}

void ServiceMapper::addServiceDialog()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Service", "Enter service name (e.g. TIKTOK):",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString s = name.trimmed().toUpper();
        if (m_mapping.contains(s)) {
            statusBar()->showMessage("Service already exists!", 3000);
            return;
        }
        m_mapping[s] = m_regionNames.isEmpty() ? "" : m_regionNames.first();
        m_serviceList->addItem(s);
        m_serviceList->setCurrentRow(m_serviceList->count() - 1);
        statusBar()->showMessage("Service added: " + s, 3000);
    }
}

void ServiceMapper::renameServiceDialog()
{
    QListWidgetItem* item = m_serviceList->currentItem();
    if (!item) return;
    QString oldName = item->text();
    bool ok;
    QString name = QInputDialog::getText(this, "Rename Service", "New name for " + oldName + ":",
                                         QLineEdit::Normal, oldName, &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString s = name.trimmed().toUpper();
        if (s != oldName && m_mapping.contains(s)) {
            statusBar()->showMessage("Service already exists!", 3000);
            return;
        }
        m_mapping[s] = m_mapping.take(oldName);
        item->setText(s);
        statusBar()->showMessage("Renamed to: " + s, 3000);
    }
}

void ServiceMapper::deleteService()
{
    QListWidgetItem* item = m_serviceList->currentItem();
    if (!item) return;
    QString svc = item->text();
    if (QMessageBox::question(this, "Delete", "Delete service " + svc + "?") == QMessageBox::Yes) {
        m_mapping.remove(svc);
        delete m_serviceList->takeItem(m_serviceList->currentRow());
        statusBar()->showMessage("Deleted: " + svc, 3000);
    }
}

void ServiceMapper::saveServices()
{
    QJsonObject obj;
    for (auto it = m_mapping.begin(); it != m_mapping.end(); ++it) {
        obj[it.key()] = it.value();
    }

    QString path = resolveServicesPath();
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        statusBar()->showMessage("Saved → " + path, 4000);
        m_pathLabel->setText("  Saved: " + path);
    } else {
        QMessageBox::critical(this, "Error", "Could not save to " + path);
    }
}

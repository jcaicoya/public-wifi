#pragma once

#include <QDialog>
#include "ShowConfig.h"

class QCheckBox;
class QComboBox;
class QGroupBox;
class QKeyEvent;
class QLabel;

class InitScreen : public QDialog
{
    Q_OBJECT

public:
    explicit InitScreen(QWidget* parent = nullptr);

    void setConfig(const ShowConfig& config);
    ShowConfig config() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onModeChanged();

private:
    void buildUi();

    QComboBox*  m_runModeCombo = nullptr;
    QLabel*     m_statusLabel  = nullptr;
    QGroupBox*  m_demoGroup    = nullptr;
    QCheckBox*  m_actSeqCheck  = nullptr;
};

#pragma once

#include <QMainWindow>
#include <QHash>
#include <QStringList>

class QListWidget;
class QComboBox;
class QLabel;

class ServiceMapper : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServiceMapper(QWidget* parent = nullptr);

private slots:
    void onServiceSelected(int row);
    void onRegionChanged(int index);
    void addServiceDialog();
    void renameServiceDialog();
    void deleteService();
    void saveServices();

private:
    void buildUi();
    void loadData();
    QString resolveRegionsPath() const;
    QString resolveServicesPath() const;

    QListWidget* m_serviceList = nullptr;
    QComboBox*   m_regionCombo = nullptr;
    QLabel*      m_pathLabel   = nullptr;

    QHash<QString, QString> m_mapping; // Service -> Region
    QStringList m_regionNames;
};

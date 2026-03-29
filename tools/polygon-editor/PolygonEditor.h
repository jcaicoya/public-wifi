#pragma once

#include <QMainWindow>

class QLabel;
class QListWidget;
class QTextEdit;
class MapCanvas;

class PolygonEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit PolygonEditor(QWidget* parent = nullptr);

private slots:
    void onRegionSelected(int row);
    void onPolygonChanged();
    void copyRegion();
    void copyAll();
    void saveRegions();
    void addRegionDialog();
    void renameRegionDialog(const QString& oldName);
    void deleteRegion(const QString& name);
    void showListContextMenu(const QPoint& pos);

private:
    void buildUi();
    QString resolveSavePath() const;

    MapCanvas*   m_canvas     = nullptr;
    QListWidget* m_regionList = nullptr;
    QTextEdit*   m_codeOutput = nullptr;
    QLabel*      m_pathLabel  = nullptr;
};

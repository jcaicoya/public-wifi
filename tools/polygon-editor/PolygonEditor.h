#pragma once

#include <QMainWindow>

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

private:
    void buildUi();

    MapCanvas*   m_canvas     = nullptr;
    QListWidget* m_regionList = nullptr;
    QTextEdit*   m_codeOutput = nullptr;
};

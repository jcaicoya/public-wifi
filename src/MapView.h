#pragma once

#include <QWidget>
#include <QHash>
#include <QList>
#include <QPixmap>
#include <QPointF>
#include <QString>
#include <QPainterPath>

class QTimer;
class QSvgRenderer;

struct MapConnection {
    QString service;
    QString targetRegion;
    QPointF start;
    QPointF end;
    qreal progress; // 0.0 to 1.0
};

class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(QWidget* parent = nullptr);
    ~MapView() override = default;

public slots:
    void addConnection(const QString& eventType);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateAnimations();

private:
    void tryLoadRegionsFromFile();
    void tryLoadServicesFromFile();
    static QPointF svgCoord(qreal lon, qreal lat);

    QHash<QString, QPainterPath> m_regions;
    QHash<QString, qreal>        m_regionHighlights;
    QHash<QString, QString>      m_regionLabels;    // region → last service that landed there
    QHash<QString, int>          m_regionPulseTicks;
    QHash<QString, QString>      m_serviceToRegion;
    
    QList<MapConnection> m_connections;
    QTimer* m_animationTimer;
    int m_pulsePhase = 0;
    QPointF m_phonePos;
    QRectF  m_contentRect; // Center-aligned 1000x600 virtual box in screen space

    QSvgRenderer* m_svgRenderer = nullptr;
    QPixmap m_bgPixmap;         // SVG + grid combined, rebuilt on stable resize
    QTimer*  m_rebuildTimer = nullptr;  // debounce for resizeEvent

    void rebuildBackground();
};

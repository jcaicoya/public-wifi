#pragma once

#include <QWidget>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QString>
#include <QPainterPath>

class QTimer;

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

private slots:
    void updateAnimations();

private:
    void buildMapRegions();

    QHash<QString, QPainterPath> m_regions;
    QHash<QString, qreal> m_regionHighlights;
    QHash<QString, QString> m_serviceToRegion;
    
    QList<MapConnection> m_connections;
    QTimer* m_animationTimer;
    int m_pulsePhase = 0;
    QPointF m_phonePos;
};

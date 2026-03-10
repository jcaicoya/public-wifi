#pragma once

#include <QWidget>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QString>

class QTimer;

struct MapConnection {
    QString service;
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
    QHash<QString, QPointF> m_serviceLocations;
    QList<MapConnection> m_connections;
    QTimer* m_animationTimer;
    int m_pulsePhase = 0;
};

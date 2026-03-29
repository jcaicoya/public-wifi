#pragma once

#include <QWidget>
#include <QHash>
#include <QVector>
#include <QPointF>
#include <QPixmap>
#include <QStringList>

class QSvgRenderer;

class MapCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit MapCanvas(QWidget* parent = nullptr);

    const QStringList& regionNames() const { return m_regionNames; }
    void setActiveRegion(const QString& name);
    const QString& activeRegion() const { return m_activeRegion; }

    void addRegion(const QString& name);
    void renameRegion(const QString& oldName, const QString& newName);
    void deleteRegion(const QString& name);

    QString generateCode(const QString& regionName) const;
    QString generateAllCode() const;

    bool saveToJson(const QString& filePath) const;
    bool loadFromJson(const QString& filePath);

    // Same projection as MapView — shared formula
    static QPointF svgCoord(qreal lon, qreal lat);
    static QPointF virtualToLonLat(const QPointF& vp);

signals:
    void polygonChanged();
    void statusChanged(const QString& msg);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    QPointF toVirtual(const QPointF& screenPt) const;
    QPointF toScreen(const QPointF& virtualPt) const;

    // Returns index of nearest node in screen-space; sets *outDist if provided
    int findNearestNode(const QVector<QPointF>& poly,
                        const QPointF& screenPt,
                        qreal* outDist = nullptr) const;

    // Returns the edge index i such that the new node should be inserted
    // between poly[i] and poly[i+1]; sets *outInsertPt (screen coords) if provided
    int findInsertEdge(const QVector<QPointF>& poly,
                       const QPointF& screenPt,
                       QPointF* outInsertPt = nullptr) const;

    void updateHover(const QPointF& screenPt);
    void rebuildBackground();
    void initRegions();

    QHash<QString, QVector<QPointF>> m_regions;  // virtual 0-1000 / 0-600
    QStringList m_regionNames;                    // display order
    QString m_activeRegion;

    // Interaction state
    int     m_dragNodeIdx  = -1;
    int     m_hoverNodeIdx = -1;
    int     m_hoverEdgeIdx = -1;
    QPointF m_hoverInsertPt;   // screen-space preview dot position

    QSvgRenderer* m_svgRenderer = nullptr;
    QPixmap       m_bgPixmap;
    QRectF        m_contentRect; // Center-aligned 1000x600 box in screen space

    static constexpr qreal NODE_R = 6.0;   // screen pixels, node display radius
    static constexpr qreal HIT_R  = 11.0;  // screen pixels, hit-test radius
};

#include "MapCanvas.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <QFile>
#include <cmath>
#include <limits>

// ── Projection (identical to MapView::svgCoord) ───────────────────────────────

QPointF MapCanvas::svgCoord(qreal lon, qreal lat)
{
    const qreal xs = (lon + 180.0) * 7.024;
    const qreal ys = (90.0 - lat)  * 8.35;
    return { (xs - 82.992) / 2528.57 * 1000.0,
             (ys - 45.607) / 1238.92 * 600.0 };
}

QPointF MapCanvas::virtualToLonLat(const QPointF& vp)
{
    qreal lon = (vp.x() * 2528.57 / 1000.0 + 82.992) / 7.024 - 180.0;
    qreal lat = 90.0 - (vp.y() * 1238.92 / 600.0 + 45.607) / 8.35;
    return { lon, lat };
}

// ── Constructor & initialisation ──────────────────────────────────────────────

MapCanvas::MapCanvas(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(800, 480);

    QFile f(":/world_map.svg");
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray svg = f.readAll();
        // Same restyling as the main app — borders-only, cyan-blue
        svg.replace("fill:#e0e0e0",    "fill:none");
        svg.replace("fill: #e0e0e0",   "fill:none");
        svg.replace("stroke:#ffffff",  "stroke:#1a8cbf");
        svg.replace("stroke:#000000",  "stroke:#1a8cbf");
        svg.replace("stroke-width:0.5","stroke-width:3.0");
        svg.replace("stroke-width:0.3","stroke-width:2.0");
        m_svgRenderer = new QSvgRenderer(svg, this);
    }

    initRegions();
    if (!m_regionNames.isEmpty())
        setActiveRegion(m_regionNames.first());
}

void MapCanvas::initRegions()
{
    // Initial polygons mirror MapView::buildMapRegions() exactly.
    // Edit here to reset, or just use the tool to adjust interactively.
    auto gp = [](qreal lon, qreal lat) { return svgCoord(lon, lat); };

    m_regionNames = {
        "North America", "South America", "Western Europe", "Northern Europe",
        "Russia / CIS",  "Africa",        "Middle East",   "South Asia",
        "East Asia",     "Oceania"
    };

    m_regions["North America"] = {
        gp(-168, 66), gp(-155, 60), gp(-164, 54), gp(-123, 49),
        gp(-118, 34), gp( -99, 19), gp( -79,  9), gp( -80, 26),
        gp( -74, 41), gp( -63, 45), gp( -53, 47), gp( -56, 53),
        gp( -80, 63), gp(-100, 72), gp(-130, 72),
    };
    m_regions["South America"] = {
        gp(-67, 10), gp(-35, -8),  gp(-43,-23), gp(-58,-34),
        gp(-66,-52), gp(-77,-12),  gp(-76,  5),
    };
    m_regions["Western Europe"] = {
        gp(-6, 53), gp(10, 54), gp(16, 48), gp( 9, 45),
        gp( 2, 41), gp(-5, 36), gp(-9, 38),
    };
    m_regions["Northern Europe"] = {
        gp(10, 54), gp(20, 55), gp(33, 69), gp(26, 71),
        gp(18, 69), gp(10, 63), gp(10, 60), gp(18, 59), gp(25, 60),
    };
    m_regions["Russia / CIS"] = {
        gp( 20, 55), gp( 37, 56), gp( 49, 46), gp( 69, 41),
        gp( 83, 55), gp(132, 43), gp(163, 53), gp(130, 62),
        gp(130, 72), gp( 70, 73), gp( 30, 72), gp( 33, 69),
    };
    m_regions["Africa"] = {
        gp(-16, 28), gp(-6,  34), gp(10, 37), gp(32, 30),
        gp( 43, 11), gp(45,   2), gp(40,-11), gp(33,-26),
        gp( 18,-34), gp(13,  -9), gp( 3,  6), gp( 0,  6), gp(-17, 15),
    };
    m_regions["Middle East"] = {
        gp(29, 41), gp(51, 36), gp(58, 23), gp(45, 13),
        gp(43, 11), gp(44, 33), gp(36, 34),
    };
    m_regions["South Asia"] = {
        gp(62, 36), gp(77, 29), gp(88, 22),
        gp(80,  7), gp(73, 19), gp(67, 25),
    };
    m_regions["East Asia"] = {
        gp(100, 55), gp(135, 50), gp(127, 37), gp(121, 31), gp(114, 22),
        gp(121, 14), gp(107, -6), gp(104,  1), gp(101, 14), gp(102, 24),
        gp(116, 40),
    };
    m_regions["Oceania"] = {
        gp(116,-32), gp(131,-12), gp(153,-27),
        gp(151,-34), gp(145,-38), gp(138,-35),
    };
}

void MapCanvas::setActiveRegion(const QString& name)
{
    m_activeRegion = name;
    m_dragNodeIdx  = -1;
    m_hoverNodeIdx = -1;
    m_hoverEdgeIdx = -1;
    update();
}

// ── Coordinate helpers ────────────────────────────────────────────────────────

QPointF MapCanvas::toVirtual(const QPointF& sp) const
{
    if (width() == 0 || height() == 0) return {};
    return { sp.x() * 1000.0 / width(), sp.y() * 600.0 / height() };
}

QPointF MapCanvas::toScreen(const QPointF& vp) const
{
    return { vp.x() * width() / 1000.0, vp.y() * height() / 600.0 };
}

int MapCanvas::findNearestNode(const QVector<QPointF>& poly,
                               const QPointF& sp,
                               qreal* outDist) const
{
    int   best  = -1;
    qreal bestD = std::numeric_limits<qreal>::max();
    for (int i = 0; i < poly.size(); ++i) {
        QPointF d = toScreen(poly[i]) - sp;
        qreal dist = std::sqrt(d.x()*d.x() + d.y()*d.y());
        if (dist < bestD) { bestD = dist; best = i; }
    }
    if (outDist) *outDist = bestD;
    return best;
}

int MapCanvas::findInsertEdge(const QVector<QPointF>& poly,
                              const QPointF& sp,
                              QPointF* outInsertPt) const
{
    if (poly.size() < 2) return 0;

    int     bestIdx  = 0;
    qreal   bestDist = std::numeric_limits<qreal>::max();
    QPointF bestPt;

    for (int i = 0; i < poly.size(); ++i) {
        QPointF a  = toScreen(poly[i]);
        QPointF b  = toScreen(poly[(i + 1) % poly.size()]);
        QPointF ab = b - a;
        qreal len2 = ab.x()*ab.x() + ab.y()*ab.y();
        if (len2 < 1e-9) continue;

        qreal t = ((sp.x()-a.x())*ab.x() + (sp.y()-a.y())*ab.y()) / len2;
        t = qBound(0.0, t, 1.0);
        QPointF closest = a + t * ab;

        QPointF diff = sp - closest;
        qreal dist = std::sqrt(diff.x()*diff.x() + diff.y()*diff.y());
        if (dist < bestDist) { bestDist = dist; bestIdx = i; bestPt = closest; }
    }

    if (outInsertPt) *outInsertPt = bestPt;
    return bestIdx;
}

void MapCanvas::updateHover(const QPointF& sp)
{
    if (m_activeRegion.isEmpty() || !m_regions.contains(m_activeRegion)) return;
    const auto& poly = m_regions[m_activeRegion];

    qreal nodeDist;
    int nodeIdx = findNearestNode(poly, sp, &nodeDist);

    if (nodeDist <= HIT_R) {
        m_hoverNodeIdx = nodeIdx;
        m_hoverEdgeIdx = -1;
    } else {
        m_hoverNodeIdx = -1;
        m_hoverEdgeIdx = findInsertEdge(poly, sp, &m_hoverInsertPt);
    }
}

// ── Mouse events ──────────────────────────────────────────────────────────────

void MapCanvas::mousePressEvent(QMouseEvent* ev)
{
    if (m_activeRegion.isEmpty() || !m_regions.contains(m_activeRegion)) return;
    auto& poly = m_regions[m_activeRegion];
    const QPointF sp = ev->position();

    if (ev->button() == Qt::LeftButton) {
        qreal nodeDist;
        int nodeIdx = findNearestNode(poly, sp, &nodeDist);

        if (nodeDist <= HIT_R) {
            // Start dragging this node
            m_dragNodeIdx = nodeIdx;
        } else {
            // Insert new node on the nearest edge
            QPointF insertSp;
            int edgeIdx = findInsertEdge(poly, sp, &insertSp);
            poly.insert(edgeIdx + 1, toVirtual(insertSp));
            m_dragNodeIdx  = edgeIdx + 1;
            m_hoverEdgeIdx = -1;
            emit polygonChanged();
        }

    } else if (ev->button() == Qt::RightButton) {
        qreal nodeDist;
        int nodeIdx = findNearestNode(poly, sp, &nodeDist);
        if (nodeDist <= HIT_R && poly.size() > 3) {
            poly.remove(nodeIdx);
            m_hoverNodeIdx = -1;
            m_dragNodeIdx  = -1;
            emit polygonChanged();
        }
    }
    update();
}

void MapCanvas::mouseMoveEvent(QMouseEvent* ev)
{
    const QPointF sp = ev->position();

    if (m_dragNodeIdx >= 0 && m_regions.contains(m_activeRegion)) {
        auto& poly = m_regions[m_activeRegion];
        if (m_dragNodeIdx < poly.size()) {
            poly[m_dragNodeIdx] = toVirtual(sp);
            emit polygonChanged();
        }
    } else {
        updateHover(sp);
    }

    // Status bar: show virtual and geographic coordinates
    QPointF vp = toVirtual(sp);
    QPointF ll = virtualToLonLat(vp);
    emit statusChanged(
        QString("  virtual (%1, %2)   |   lon %3°  lat %4°   |   "
                "Left-click: add/drag node   Right-click: delete node")
        .arg(qRound(vp.x()), 4)
        .arg(qRound(vp.y()), 4)
        .arg(ll.x(), 6, 'f', 1)
        .arg(ll.y(), 6, 'f', 1)
    );

    update();
}

void MapCanvas::mouseReleaseEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton && m_dragNodeIdx >= 0) {
        m_dragNodeIdx = -1;
        emit polygonChanged();
        updateHover(ev->position());
        update();
    }
}

void MapCanvas::leaveEvent(QEvent*)
{
    m_hoverNodeIdx = -1;
    m_hoverEdgeIdx = -1;
    update();
}

// ── Background ────────────────────────────────────────────────────────────────

void MapCanvas::rebuildBackground()
{
    if (!m_svgRenderer || width() == 0 || height() == 0) return;
    m_bgPixmap = QPixmap(size());
    m_bgPixmap.fill(QColor("#090C10"));
    QPainter p(&m_bgPixmap);
    p.setRenderHint(QPainter::Antialiasing);
    m_svgRenderer->render(&p, QRectF(0, 0, width(), height()));
}

void MapCanvas::resizeEvent(QResizeEvent* ev)
{
    QWidget::resizeEvent(ev);
    rebuildBackground();
}

// ── Painting ──────────────────────────────────────────────────────────────────

void MapCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    if (m_bgPixmap.isNull()) rebuildBackground();
    if (!m_bgPixmap.isNull())
        p.drawPixmap(0, 0, m_bgPixmap);
    else
        p.fillRect(rect(), QColor("#090C10"));

    auto buildPath = [&](const QVector<QPointF>& pts) {
        QPainterPath path;
        if (pts.isEmpty()) return path;
        path.moveTo(toScreen(pts[0]));
        for (int i = 1; i < pts.size(); ++i)
            path.lineTo(toScreen(pts[i]));
        path.closeSubpath();
        return path;
    };

    // ── Inactive regions (dim teal) ───────────────────────────────────────────
    for (const QString& name : m_regionNames) {
        if (name == m_activeRegion) continue;
        p.setBrush(QColor(0, 160, 160, 22));
        p.setPen(QPen(QColor(0, 160, 160, 90), 1.2));
        p.drawPath(buildPath(m_regions[name]));
    }

    // ── Active region ─────────────────────────────────────────────────────────
    if (!m_activeRegion.isEmpty() && m_regions.contains(m_activeRegion)) {
        const auto& poly = m_regions[m_activeRegion];

        // Filled polygon
        p.setBrush(QColor(0, 255, 255, 30));
        p.setPen(Qt::NoPen);
        p.drawPath(buildPath(poly));

        // Edges — yellow if it's the hover edge (insertion preview), cyan otherwise
        for (int i = 0; i < poly.size(); ++i) {
            QPointF a = toScreen(poly[i]);
            QPointF b = toScreen(poly[(i + 1) % poly.size()]);
            bool hover = (i == m_hoverEdgeIdx && m_hoverNodeIdx < 0 && m_dragNodeIdx < 0);
            p.setPen(QPen(hover ? QColor(255, 220, 0) : QColor(0, 255, 255), hover ? 2.5 : 1.5));
            p.drawLine(a, b);
        }

        // Insertion preview dot on hovered edge
        if (m_hoverEdgeIdx >= 0 && m_hoverNodeIdx < 0 && m_dragNodeIdx < 0) {
            p.setBrush(QColor(255, 220, 0, 210));
            p.setPen(QPen(QColor(255, 220, 0), 1.5));
            p.drawEllipse(m_hoverInsertPt, NODE_R, NODE_R);
        }

        // Node handles
        QFont labelFont;
        labelFont.setPointSize(6);
        labelFont.setBold(true);
        p.setFont(labelFont);

        for (int i = 0; i < poly.size(); ++i) {
            QPointF sp = toScreen(poly[i]);
            bool isHover = (i == m_hoverNodeIdx);
            bool isDrag  = (i == m_dragNodeIdx);

            // Outer glow for active/hover nodes
            if (isHover || isDrag) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(isDrag ? QColor(255, 100, 80, 120) : QColor(255, 220, 0, 120), 6.0));
                p.drawEllipse(sp, NODE_R + 3, NODE_R + 3);
            }

            // Node circle
            QColor fill = isDrag  ? QColor(255, 110, 90)
                        : isHover ? QColor(255, 220,  0)
                                  : QColor(255, 255, 255);
            p.setBrush(fill);
            p.setPen(QPen(QColor(0, 200, 200), 1.5));
            p.drawEllipse(sp, NODE_R, NODE_R);

            // Index label inside the circle
            p.setPen(QColor(20, 20, 20));
            p.drawText(QRectF(sp.x()-NODE_R, sp.y()-NODE_R, NODE_R*2, NODE_R*2),
                       Qt::AlignCenter, QString::number(i));
        }
    }
}

// ── Code generation ───────────────────────────────────────────────────────────

QString MapCanvas::generateCode(const QString& regionName) const
{
    if (!m_regions.contains(regionName)) return {};
    const auto& poly = m_regions[regionName];

    QString code;
    code += QString("// ---- %1 ----  [%2 nodes]\n").arg(regionName).arg(poly.size());
    code += QString("m_regions[\"%1\"] = makePath({\n").arg(regionName);
    for (const QPointF& vp : poly) {
        QPointF ll = virtualToLonLat(vp);
        code += QString("    gp(%1, %2),  // (%3, %4)\n")
                    .arg(ll.x(), 7, 'f', 1)
                    .arg(ll.y(), 6, 'f', 1)
                    .arg(qRound(vp.x()), 4)
                    .arg(qRound(vp.y()), 4);
    }
    code += "});\n";
    return code;
}

QString MapCanvas::generateAllCode() const
{
    QString all;
    for (const QString& name : m_regionNames) {
        all += generateCode(name);
        all += "\n";
    }
    return all;
}

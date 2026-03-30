#include "MapView.h"

namespace {
QColor serviceColor(const QString& service)
{
    static const QHash<QString, QColor> kColors = {
        { "BANKING",   QColor("#FF3333") },
        { "EMAIL",     QColor("#FF6644") },
        { "VPN",       QColor("#FF3333") },
        { "WHATSAPP",  QColor("#00FFFF") },
        { "SOCIAL",    QColor("#FFD700") },
        { "SHOPPING",  QColor("#FFB300") },
        { "AMAZON",    QColor("#FFB300") },
        { "VIDEO",     QColor("#FF9900") },
        { "STREAMING", QColor("#FF9900") },
        { "GAMING",    QColor("#AA66FF") },
        { "MAPS",      QColor("#88CCFF") },
        { "REGIONAL",  QColor("#88CCFF") },
        { "LATAM",     QColor("#88CCFF") },
        { "PACIFIC",   QColor("#88CCFF") },
        { "TELEMETRY", QColor("#888888") },
        { "CDN",       QColor("#888888") },
        { "MICROSOFT", QColor("#AAAAAA") },
        { "APPLE",     QColor("#AAAAAA") },
        { "SEARCH",    QColor("#00FF44") },
        { "NEWS",      QColor("#00FF44") },
    };
    return kColors.value(service, QColor("#CCCCCC"));
}
} // namespace

#include <QPainter>
#include <QPen>
#include <QColor>
#include <QTimer>
#include <QFont>
#include <QFile>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    // Phone location: Oviedo, Asturias, Spain (lon=-5.845°, lat=43.361°).
    m_phonePos = svgCoord(-5.845, 43.361);

    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &MapView::updateAnimations);
    m_animationTimer->start(30); // ~33 FPS

    m_rebuildTimer = new QTimer(this);
    m_rebuildTimer->setSingleShot(true);
    connect(m_rebuildTimer, &QTimer::timeout, this, &MapView::rebuildBackground);

    // Load the SVG world map, restyle it for the cyber theme, and pre-render once.
    // Borders-only look: land fill removed, borders drawn in dim cyan-blue.
    // Stroke widths boosted so they are visible at 1000×600 virtual resolution.
    QFile svgFile(QStringLiteral(":/world_map.svg"));
    if (svgFile.open(QIODevice::ReadOnly)) {
        QByteArray svgData = svgFile.readAll();
        svgData.replace("fill:#e0e0e0", "fill:none");
        svgData.replace("fill: #e0e0e0", "fill:none");
        svgData.replace("fill:#ffffff", "fill:none");
        svgData.replace("fill: #ffffff", "fill:none");
        svgData.replace("stroke:#ffffff", "stroke:#1a8cbf");
        svgData.replace("stroke:#000000", "stroke:#1a8cbf");
        svgData.replace("stroke-width:0.5", "stroke-width:3.0");
        svgData.replace("stroke-width:0.3", "stroke-width:2.0");
        m_svgRenderer = new QSvgRenderer(svgData, this);
        // m_bgPixmap is built in resizeEvent / rebuildBackground() once the widget has a size.
    }

    tryLoadRegionsFromFile();
    tryLoadServicesFromFile();
}

void MapView::tryLoadServicesFromFile()
{
    QStringList candidates = {
        QDir::currentPath() + "/resources/services.json",
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../resources/services.json"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../resources/services.json"),
        QCoreApplication::applicationDirPath() + "/services.json",
    };

    QString path;
    for (const QString& c : candidates) {
        if (QFile::exists(c)) { path = c; break; }
    }
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    if (obj.isEmpty()) return;

    m_serviceToRegion.clear();
    for (const QString& svc : obj.keys()) {
        m_serviceToRegion[svc.toUpper()] = obj[svc].toString();
    }
    qDebug() << "MapView: loaded" << m_serviceToRegion.size() << "service mappings from" << path;
}

void MapView::tryLoadRegionsFromFile()
{
    // Candidate paths, tried in order:
    //  1. Working directory / resources / regions.json  (CLion: set working dir to project root)
    //  2. Relative to executable (for deployed or differently-configured builds)
    QStringList candidates = {
        QDir::currentPath() + "/resources/regions.json",
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../resources/regions.json"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../resources/regions.json"),
        QCoreApplication::applicationDirPath() + "/regions.json",
    };

    QString path;
    for (const QString& c : candidates) {
        if (QFile::exists(c)) { path = c; break; }
    }
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;

    QJsonObject regionsObj = doc.object()["regions"].toObject();
    if (regionsObj.isEmpty()) return;

    auto makePath = [](const QVector<QPointF>& pts) -> QPainterPath {
        QPainterPath p;
        if (pts.isEmpty()) return p;
        p.moveTo(pts[0]);
        for (int i = 1; i < pts.size(); ++i) p.lineTo(pts[i]);
        p.closeSubpath();
        return p;
    };

    for (const QString& name : regionsObj.keys()) {
        QJsonArray arr = regionsObj[name].toArray();
        QVector<QPointF> pts;
        for (const QJsonValue& v : arr) {
            QJsonArray pt = v.toArray();
            if (pt.size() == 2)
                pts << svgCoord(pt[0].toDouble(), pt[1].toDouble());
        }
        if (pts.size() >= 3) {
            m_regions[name]          = makePath(pts);
            m_regionHighlights[name] = 0.0;
        }
    }

    qDebug() << "MapView: loaded regions from" << path;
}

QPointF MapView::svgCoord(qreal lon, qreal lat)
{
    // Converts geographic (lon, lat) to our 1000×600 virtual coordinate space,
    // aligned with how the SVG world map renders.
    //
    // The SVG (BlankMap-World-Compact) uses a pseudo-equirectangular projection
    // with different x/y scales, empirically calibrated from Spain's path data:
    //   x_svg = (lon + 180) × 7.024     [scale = viewBox_w / 360° = 2528.57/360]
    //   y_svg = (90 − lat)  × 8.35      [calibrated: Spain NW coast path at SVG y=389
    //                                     corresponds to ~43.6°N → 46.4° from pole
    //                                     → 389/46.4 ≈ 8.38; map spans ~84°N to ~65°S]
    //
    // SVG viewBox: origin (82.992, 45.607), size 2528.57 × 1238.92
    // Maps to our virtual space: (0,0)–(1000,600)
    const qreal xs = (lon + 180.0) * 7.024;
    const qreal ys = (90.0 - lat)  * 8.35;
    return {
        (xs - 82.992)  / 2528.57 * 1000.0,
        (ys - 45.607)  / 1238.92 * 600.0
    };
}


void MapView::addConnection(const QString& eventType)
{
    QString svc = eventType.toUpper();
    if (!m_serviceToRegion.contains(svc)) {
        // If not in mapping, we ignore it as requested.
        return;
    }

    MapConnection conn;
    conn.service = svc;
    
    // Look up the target region using our dynamic dictionary.
    conn.targetRegion = m_serviceToRegion.value(svc); 
    
    conn.start = m_phonePos;
    
    // Target the center of the region's bounding box
    if (m_regions.contains(conn.targetRegion)) {
        conn.end = m_regions[conn.targetRegion].boundingRect().center();
    } else {
        conn.end = QPointF(500, 300); // Fallback to center of map
    }
    
    conn.progress = 0.0;
    m_connections.append(conn);
}

void MapView::updateAnimations()
{
    m_pulsePhase = (m_pulsePhase + 1) % 40;

    bool dirty = false; // track whether anything actually changed

    // Fade out region highlights over time
    for (auto it = m_regionHighlights.begin(); it != m_regionHighlights.end(); ++it) {
        if (it.value() > 0.0) {
            it.value() -= 0.03;
            if (it.value() < 0.0) it.value() = 0.0;
            dirty = true;
        }
    }

    // Move packets along the lines
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        m_connections[i].progress += 0.015; // Packet speed
        dirty = true;

        if (m_connections[i].progress >= 1.0) {
            const QString region = m_connections[i].targetRegion;
            m_regionHighlights[region] = 1.0;
            m_regionLabels[region]     = m_connections[i].service;
            m_connections.removeAt(i);
        }
    }
    
    // Pulse ring always animates; skip repaint only when the map is fully idle
    if (dirty || !m_connections.isEmpty())
        update();
    else if (m_pulsePhase % 4 == 0)   // still repaint for pulse ring, but at 8 fps when idle
        update();
}

void MapView::rebuildBackground()
{
    if (!m_svgRenderer || width() == 0 || height() == 0) return;

    // Calculate Aspect Fit rect for 1000x600 virtual space
    qreal widgetRatio = qreal(width()) / height();
    qreal mapRatio    = 1000.0 / 600.0;
    qreal w, h;

    if (widgetRatio > mapRatio) {
        h = height();
        w = h * mapRatio;
    } else {
        w = width();
        h = w / mapRatio;
    }
    m_contentRect = QRectF((width() - w) / 2.0, (height() - h) / 2.0, w, h);

    // Render SVG + grid into a single pixmap at the real widget resolution.
    m_bgPixmap = QPixmap(width(), height());
    m_bgPixmap.fill(QColor("#090C10"));

    QPainter p(&m_bgPixmap);
    p.setRenderHint(QPainter::Antialiasing);

    // SVG at centered content resolution
    m_svgRenderer->render(&p, m_contentRect);

    // Grid in virtual 1000×600 space, scaled and translated to match m_contentRect
    p.save();
    p.translate(m_contentRect.topLeft());
    p.scale(m_contentRect.width() / 1000.0, m_contentRect.height() / 600.0);
    
    QPen gridPen(QColor(30, 40, 60, 55));
    gridPen.setWidthF(0.8);
    gridPen.setStyle(Qt::DotLine);
    p.setPen(gridPen);
    for (int x = 0; x <= 1000; x += 50) p.drawLine(x, 0, x, 600);
    for (int y = 0; y <= 600; y += 50) p.drawLine(0, y, 1000, y);
    p.restore();
}

void MapView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_rebuildTimer->start(150); // debounce: rebuild only after resize settles
}

void MapView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- Layer 1+2: background (SVG + grid) ---
    if (!m_bgPixmap.isNull() && m_bgPixmap.size() == size())
        painter.drawPixmap(0, 0, m_bgPixmap);
    else
        painter.fillRect(rect(), QColor("#090C10"));

    painter.save();
    // Translate and scale to match the centered content rect
    painter.translate(m_contentRect.topLeft());
    painter.scale(m_contentRect.width() / 1000.0, m_contentRect.height() / 600.0);
    
    // Clip to virtual space so elements don't draw in letterbox areas
    painter.setClipRect(0, 0, 1000, 600);

    // --- Layer 3: region highlight overlays ---
    // Idle regions are fully transparent — the SVG handles the visual.
    // Polygons only appear as glowing cyan overlays when a packet arrives.
    QFont labelFont("Consolas", 10, QFont::Bold);
    painter.setFont(labelFont);
    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        const QString& regionName = it.key();
        const QPainterPath& path = it.value();
        qreal highlight = m_regionHighlights.value(regionName, 0.0);

        if (highlight > 0.0) {
            QColor fillColor(0, 255, 255, static_cast<int>(120 * highlight));
            QColor edgeColor(0, 255, 255, static_cast<int>(255 * highlight));

            painter.setBrush(fillColor);
            QPen regionPen(edgeColor);
            regionPen.setWidthF(2.5);
            painter.setPen(regionPen);
            painter.drawPath(path);

            const QPointF center = path.boundingRect().center();
            const QString service = m_regionLabels.value(regionName);

            // Service name — large, colored, fades with highlight
            if (!service.isEmpty()) {
                QColor svcColor = serviceColor(service);
                svcColor.setAlpha(static_cast<int>(255 * highlight));

                // Dark background pill for legibility at projector distance
                QColor bgColor(0, 0, 0, static_cast<int>(160 * highlight));
                painter.setBrush(bgColor);
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(QRectF(center.x() - 68, center.y() - 20, 136, 26), 4, 4);

                QFont svcFont(QStringLiteral("Consolas"), 14, QFont::Bold);
                painter.setFont(svcFont);
                painter.setPen(svcColor);
                painter.drawText(QRectF(center.x() - 68, center.y() - 20, 136, 26),
                                 Qt::AlignCenter, service);
            }

            // Region name — small, dim, below service label
            QFont regionFont(QStringLiteral("Consolas"), 8);
            painter.setFont(regionFont);
            painter.setPen(QColor(200, 200, 200, static_cast<int>(160 * highlight)));
            painter.drawText(QRectF(center.x() - 70, center.y() + 8, 140, 16),
                             Qt::AlignCenter, regionName);
        }
    }

    // Draw the animated data connections
    for (const auto& conn : m_connections) {
        // Track line
        QPen linePen(QColor(0, 255, 200, 80)); 
        linePen.setWidthF(2.0);
        painter.setPen(linePen);
        painter.drawLine(conn.start, conn.end);

        // Particle trail (Fading tails)
        for (int j = 1; j <= 8; ++j) {
            qreal tailProgress = conn.progress - (j * 0.02);
            if (tailProgress > 0) {
                QPointF tailPos = conn.start + (conn.end - conn.start) * tailProgress;
                int alpha = 100 - (j * 12);
                if (alpha < 0) alpha = 0;
                painter.setBrush(QColor(0, 255, 255, alpha));
                painter.setPen(Qt::NoPen);
                qreal radius = 4.0 - (j * 0.4);
                if (radius < 1.0) radius = 1.0;
                painter.drawEllipse(tailPos, radius, radius);
            }
        }

        // Moving packet
        QPointF currentPos = conn.start + (conn.end - conn.start) * conn.progress;
        
        painter.setBrush(QColor("#FFFFFF")); // Core
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(currentPos, 4, 4);
        
        painter.setBrush(QColor(0, 255, 255, 120)); // Glow
        painter.drawEllipse(currentPos, 8, 8);
    }

    // Draw Fixed Phone Location (Asturias, Spain)
    painter.setBrush(QColor("#00FF44")); 
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(m_phonePos, 8, 8);
    
    // Pulsing ring around phone
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 255, 68, 150 - m_pulsePhase * 3), 2.0));
    painter.drawEllipse(m_phonePos, 8 + m_pulsePhase / 1.5, 8 + m_pulsePhase / 1.5);
    
    // Phone Label
    painter.setPen(QColor(200, 255, 200));
    QFont phoneFont("Consolas", 14, QFont::Bold);
    painter.setFont(phoneFont);
    painter.drawText(m_phonePos + QPointF(-60, 30), "ASTURIAS, SPAIN");

    painter.restore();
}

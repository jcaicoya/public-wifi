#include "MapView.h"

#include <QPainter>
#include <QPen>
#include <QColor>
#include <QTimer>
#include <QFont>

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    buildMapRegions();

    // Phone location: Asturias, Spain (Approx SW Europe on our local map coordinate system)
    m_phonePos = QPointF(440, 190);

    // Dictionary mapping Services to Geographic Regions
    m_serviceToRegion["SEARCH"]    = "North America";
    m_serviceToRegion["APPLE"]     = "North America";
    m_serviceToRegion["MICROSOFT"] = "North America";
    m_serviceToRegion["AMAZON"]    = "Western Europe";
    m_serviceToRegion["WHATSAPP"]  = "Northern Europe"; 
    m_serviceToRegion["VIDEO"]     = "East Asia";
    m_serviceToRegion["SOCIAL"]    = "North America";
    m_serviceToRegion["CDN"]       = "Western Europe";
    
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &MapView::updateAnimations);
    m_animationTimer->start(30); // ~33 FPS
}

void MapView::buildMapRegions()
{
    // Minimalist / Cyber low-poly representations of regions
    // Coordinate space: 1000x600

    // North America
    QPainterPath na;
    na.moveTo(50, 100); na.lineTo(300, 80); na.lineTo(380, 180); 
    na.lineTo(300, 280); na.lineTo(150, 280); na.lineTo(50, 200); na.closeSubpath();
    m_regions["North America"] = na;

    // South America
    QPainterPath sa;
    sa.moveTo(280, 320); sa.lineTo(380, 320); sa.lineTo(420, 420);
    sa.lineTo(350, 580); sa.lineTo(280, 450); na.closeSubpath();
    m_regions["South America"] = sa;

    // Western Europe
    QPainterPath weu;
    weu.moveTo(420, 150); weu.lineTo(480, 150); weu.lineTo(480, 220);
    weu.lineTo(420, 220); weu.closeSubpath();
    m_regions["Western Europe"] = weu;
    
    // Northern Europe
    QPainterPath neu;
    neu.moveTo(480, 50); neu.lineTo(580, 50); neu.lineTo(580, 140);
    neu.lineTo(480, 140); neu.closeSubpath();
    m_regions["Northern Europe"] = neu;

    // Eastern Europe / Russia
    QPainterPath rus;
    rus.moveTo(580, 40); rus.lineTo(950, 40); rus.lineTo(950, 150);
    rus.lineTo(580, 150); rus.closeSubpath();
    m_regions["Russia / CIS"] = rus;

    // Africa
    QPainterPath af;
    af.moveTo(420, 240); af.lineTo(580, 240); af.lineTo(620, 380);
    af.lineTo(550, 550); af.lineTo(450, 550); af.lineTo(400, 350); af.closeSubpath();
    m_regions["Africa"] = af;

    // Middle East
    QPainterPath me;
    me.moveTo(580, 200); me.lineTo(680, 200); me.lineTo(680, 280);
    me.lineTo(580, 280); me.closeSubpath();
    m_regions["Middle East"] = me;

    // East Asia (China, Japan, etc.)
    QPainterPath eas;
    eas.moveTo(750, 160); eas.lineTo(980, 160); eas.lineTo(980, 350);
    eas.lineTo(750, 350); eas.closeSubpath();
    m_regions["East Asia"] = eas;

    // South Asia (India, etc.)
    QPainterPath sas;
    sas.moveTo(680, 280); sas.lineTo(780, 280); sas.lineTo(780, 400);
    sas.lineTo(680, 400); sas.closeSubpath();
    m_regions["South Asia"] = sas;

    // Oceania
    QPainterPath oc;
    oc.moveTo(820, 420); oc.lineTo(980, 420); oc.lineTo(980, 580);
    oc.lineTo(820, 580); oc.closeSubpath();
    m_regions["Oceania"] = oc;

    // Initialize highlight intensity for all regions to 0
    for (const QString& key : m_regions.keys()) {
        m_regionHighlights[key] = 0.0;
    }
}

void MapView::addConnection(const QString& eventType)
{
    MapConnection conn;
    conn.service = eventType.isEmpty() ? "UNKNOWN" : eventType;
    
    // Look up the target region using our dictionary. Default to North America if unknown.
    conn.targetRegion = m_serviceToRegion.value(conn.service, "North America"); 
    
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

    // Fade out region highlights over time
    for (auto it = m_regionHighlights.begin(); it != m_regionHighlights.end(); ++it) {
        if (it.value() > 0.0) {
            it.value() -= 0.03; // Fade speed
            if (it.value() < 0.0) it.value() = 0.0;
        }
    }

    // Move packets along the lines
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        m_connections[i].progress += 0.015; // Packet speed
        
        if (m_connections[i].progress >= 1.0) {
            // Flash the whole region when the packet arrives!
            m_regionHighlights[m_connections[i].targetRegion] = 1.0;
            m_connections.removeAt(i);
        }
    }
    
    update();
}

void MapView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Deep cyber background
    painter.fillRect(rect(), QColor("#090C10"));

    painter.save();
    // Scale local 1000x600 coordinates to fit the actual widget size
    qreal scaleX = width() / 1000.0;
    qreal scaleY = height() / 600.0;
    painter.scale(scaleX, scaleY);

    // Draw background grid
    QPen gridPen(QColor(30, 40, 60, 150));
    gridPen.setWidthF(1.0);
    gridPen.setStyle(Qt::DotLine);
    painter.setPen(gridPen);
    for (int x = 0; x <= 1000; x += 50) painter.drawLine(x, 0, x, 600);
    for (int y = 0; y <= 600; y += 50) painter.drawLine(0, y, 1000, y);

    // Draw the World Regions (Polygons)
    for (auto it = m_regions.begin(); it != m_regions.end(); ++it) {
        const QString& regionName = it.key();
        const QPainterPath& path = it.value();
        qreal highlight = m_regionHighlights.value(regionName, 0.0);

        // Base idle colors
        QColor fillColor(20, 30, 50, 100);
        QColor edgeColor(50, 80, 120, 150);

        // Mix in highlight color (Cyan) when active
        if (highlight > 0.0) {
            int r = 20 + (0 - 20) * highlight;
            int g = 30 + (255 - 30) * highlight;
            int b = 50 + (255 - 50) * highlight;
            fillColor = QColor(r, g, b, 100 + 100 * highlight); // Brighter fill
            edgeColor = QColor(0, 255, 255, 150 + 105 * highlight); // Glowing cyan edge
        }

        painter.setBrush(fillColor);
        QPen regionPen(edgeColor);
        regionPen.setWidthF(highlight > 0 ? 3.0 : 1.0);
        painter.setPen(regionPen);
        painter.drawPath(path);

        // Draw Region Label in the center of its polygon
        painter.setPen(QColor(100, 130, 170));
        QFont font("Consolas", 10, QFont::Bold);
        painter.setFont(font);
        QPointF center = path.boundingRect().center();
        // Adjust text origin so it's roughly centered
        painter.drawText(QRectF(center.x() - 60, center.y() - 10, 120, 20), Qt::AlignCenter, regionName); 
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
        
        // Follow-text (service name flies with the packet)
        painter.setPen(QColor(255, 255, 255, 200));
        painter.drawText(currentPos + QPointF(10, -10), conn.service);
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

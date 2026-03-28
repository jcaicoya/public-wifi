#include "MapView.h"

#include <QPainter>
#include <QPen>
#include <QColor>
#include <QTimer>
#include <QFont>
#include <QFile>
#include <QSvgRenderer>

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    buildMapRegions();

    // Phone location: Oviedo, Asturias, Spain (lon=-5.845°, lat=43.361°).
    // Uses the same SVG-aligned projection as buildMapRegions().
    m_phonePos = svgCoord(-5.845, 43.361);

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

    // Load the SVG world map, restyle it for the cyber theme, and pre-render once.
    // Borders-only look: land fill removed, borders drawn in dim cyan-blue.
    // Stroke widths boosted so they are visible at 1000×600 virtual resolution.
    QFile svgFile(QStringLiteral(":/world_map.svg"));
    if (svgFile.open(QIODevice::ReadOnly)) {
        QByteArray svgData = svgFile.readAll();

        // Remove land fill (both "fill:#e0e0e0" and "fill: #e0e0e0" variants in the SVG CSS)
        svgData.replace("fill:#e0e0e0", "fill:none");
        svgData.replace("fill: #e0e0e0", "fill:none");

        // Borders: white country borders → dim cyber blue; black circles → same
        svgData.replace("stroke:#ffffff", "stroke:#1a8cbf");
        svgData.replace("stroke:#000000", "stroke:#1a8cbf");

        // Make strokes thick enough to be visible at our render resolution
        svgData.replace("stroke-width:0.5", "stroke-width:3.0");
        svgData.replace("stroke-width:0.3", "stroke-width:2.0");

        m_svgRenderer = new QSvgRenderer(svgData, this);

        // Render once at virtual 1000×600 resolution
        m_mapPixmap = QPixmap(1000, 600);
        m_mapPixmap.fill(Qt::transparent);
        QPainter pixPainter(&m_mapPixmap);
        pixPainter.setRenderHint(QPainter::Antialiasing);
        m_svgRenderer->render(&pixPainter, QRectF(0, 0, 1000, 600));
    }
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

void MapView::buildMapRegions()
{
    // All region vertices use svgCoord(lon, lat) so highlights align with the SVG map.
    auto gp = [this](qreal lon, qreal lat) -> QPointF {
        return svgCoord(lon, lat);
    };

    auto makePath = [](const QVector<QPointF>& pts) -> QPainterPath {
        QPainterPath path;
        if (pts.isEmpty()) return path;
        path.moveTo(pts[0]);
        for (int i = 1; i < pts.size(); ++i)
            path.lineTo(pts[i]);
        path.closeSubpath();
        return path;
    };

    // ---- North America ----
    // Full perimeter: SW Alaska → Arctic coast → Labrador → E coast → Florida peninsula
    // → Gulf coast → Mexico → Central America (both coasts) → Pacific coast → back north.
    // No ocean jumps: every segment follows land or near coastline.
    m_regions["North America"] = makePath({
        // SW Alaska and Pacific coast south
        gp(-168, 54), gp(-163, 55), gp(-152, 57), gp(-148, 61), gp(-136, 57),
        gp(-130, 54), gp(-124, 49), gp(-124, 42), gp(-117, 32), gp(-110, 23),
        gp(-106, 20), gp(-97,  16),
        // Central America — Pacific coast going south
        gp(-92,  14), gp(-87,  13), gp(-85,  10), gp(-79,   8),
        // Central America — Caribbean coast going north
        gp(-82,   9), gp(-83,  10), gp(-87,  16), gp(-88,  16), gp(-87,  21),
        // Gulf coast
        gp(-97,  22), gp(-97,  26), gp(-94,  29), gp(-90,  29), gp(-89,  30), gp(-84,  30),
        // Florida peninsula
        gp(-82,  28), gp(-82,  24), gp(-80,  25), gp(-80,  27), gp(-81,  30), gp(-80,  32),
        // US East coast going north
        gp(-76,  35), gp(-75,  38), gp(-74,  40), gp(-70,  42), gp(-66,  44), gp(-60,  47),
        // Arctic coast
        gp(-65,  60), gp(-78,  63), gp(-85,  72), gp(-95,  73), gp(-120, 72), gp(-140, 70),
        gp(-165, 68)
    });

    // ---- South America ----
    // N coast → Brazil's eastern bulge → Río de la Plata → Patagonia → Pacific coast back north.
    m_regions["South America"] = makePath({
        gp(-78,  12), gp(-73,  11), gp(-63,  10), gp(-52,   4),
        gp(-35,  -5),                               // Brazil NE bulge (Recife)
        gp(-38, -13), gp(-43, -23),                 // Salvador → Rio de Janeiro
        gp(-48, -28), gp(-52, -33), gp(-58, -34),   // Porto Alegre → Buenos Aires
        gp(-62, -42), gp(-65, -55), gp(-68, -54),   // Patagonia → Tierra del Fuego
        gp(-72, -50), gp(-75, -38), gp(-80,  -3), gp(-78,   8)
    });

    // ---- Western Europe ----
    // Portugal SW → Mediterranean → N Sea → English Channel → Bay of Biscay.
    m_regions["Western Europe"] = makePath({
        gp(-9,  36), gp(-5,  36), gp( 0,  38), gp( 3,  43), gp( 5,  44),
        gp( 8,  44), gp(10,  44), gp(13,  47), gp(18,  47), gp(20,  55),
        gp(14,  57), gp( 8,  55), gp( 4,  52), gp( 2,  51),
        gp(-1,  51), gp(-5,  50), gp(-4,  48), gp(-2,  47),
        gp(-1,  44), gp(-2,  43), gp(-8,  44), gp(-9,  39)
    });

    // ---- Italy ---- (the recognizable boot shape)
    m_regions["Italy"] = makePath({
        gp( 7,  44), gp( 9,  44), gp(12,  44), gp(14,  45),  // Genoa → Venice → Trieste
        gp(14,  43), gp(16,  41), gp(18,  40),                 // Adriatic coast → Heel
        gp(16,  38), gp(15,  38),                               // Toe tip
        gp(16,  39), gp(14,  40), gp(12,  42)                  // Naples → Rome coast
    });

    // ---- United Kingdom ----
    m_regions["United Kingdom"] = makePath({
        gp(-5,  50), gp( 2,  51), gp( 0,  53), gp(-1,  55),
        gp(-2,  57), gp(-3,  56), gp(-4,  58), gp(-5,  58),
        gp(-5,  57), gp(-5,  56), gp(-5,  53), gp(-4,  51), gp(-3,  50)
    });

    // ---- Ireland ----
    m_regions["Ireland"] = makePath({
        gp(-10, 51), gp(-8,  52), gp(-6,  52), gp(-6,  54),
        gp(-8,  55), gp(-10, 54), gp(-10, 52)
    });

    // ---- Northern Europe (Scandinavia + Finland) ----
    m_regions["Northern Europe"] = makePath({
        gp( 5,  57), gp( 8,  55), gp(12,  56), gp(18,  57), gp(24,  57),
        gp(28,  60), gp(30,  60), gp(28,  65), gp(25,  68),
        gp(18,  70), gp(14,  65), gp( 5,  62)
    });

    // ---- Russia / CIS ----
    // Baltic → Black Sea → Caucasus → Central Asia → Siberia → Arctic coast.
    m_regions["Russia / CIS"] = makePath({
        gp(28,  56), gp(40,  47), gp(45,  42), gp(55,  42), gp(65,  38),
        gp(80,  42), gp(100, 50), gp(130, 43), gp(140, 46),
        gp(152, 48), gp(163, 60), gp(165, 63), gp(160, 68),
        gp(140, 72), gp(100, 73), gp(70,  73), gp(50,  72),
        gp(30,  72), gp(28,  65), gp(30,  60)
    });

    // ---- Africa ----
    // NW Morocco → Mediterranean coast → Horn of Africa → E coast → Cape of Good Hope
    // → W coast → Gulf of Guinea detail → Senegal → Mauritania → Morocco.
    m_regions["Africa"] = makePath({
        gp(-6,  35), gp( 5,  37), gp(10,  37), gp(25,  34), gp(33,  32),
        gp(38,  22),                                            // Red Sea / Eritrea
        gp(43,  12), gp(51,  12), gp(51,  10),                 // Horn of Africa
        gp(44,   2), gp(40,  -2), gp(40, -11), gp(35, -18),
        gp(33, -28), gp(27, -35), gp(18, -35), gp(15, -33),   // Cape of Good Hope
        gp(12, -17), gp( 9,   0), gp( 9,   4),                 // Congo/Cameroon coast
        // Gulf of Guinea — the critical westward concave detail
        gp( 7,   4), gp( 3,   5), gp( 0,   5), gp(-5,   5), gp(-8,   5),
        gp(-13,  8), gp(-15, 10), gp(-17, 14),                 // Sierra Leone → Senegal
        gp(-18, 16), gp(-17, 21), gp(-13, 28)                  // Mauritania → S Morocco
    });

    // ---- Middle East ----
    m_regions["Middle East"] = makePath({
        gp(26,  37), gp(36,  37), gp(43,  37), gp(58,  22),
        gp(57,  15), gp(45,  12), gp(38,  22),
        gp(32,  30), gp(33,  32), gp(35,  37)
    });

    // ---- South Asia (Indian subcontinent) ----
    // Pakistan NW → Nepal/NE India → Bay of Bengal → southern tip → Malabar coast.
    m_regions["South Asia"] = makePath({
        gp(62,  38), gp(97,  35), gp(97,  22), gp(92,  22),
        gp(80,   8), gp(77,   8),               // Southern tip (Kanyakumari)
        gp(72,  20), gp(65,  22), gp(62,  28)
    });

    // ---- East Asia ----
    // Siberia/China border → Korean peninsula → China coast → SE Asia
    // → Indonesian archipelago → back north.
    m_regions["East Asia"] = makePath({
        gp(100, 55), gp(130, 50), gp(130, 43),
        gp(128, 35), gp(126, 34), gp(127, 37), gp(125, 40),    // Korean peninsula
        gp(121, 31), gp(115, 22), gp(108, 20), gp(102, 10),
        gp(103,  3),                                             // Singapore / Malay Peninsula
        gp(100, -5), gp(110, -8), gp(120, -5),                 // Sumatra → Java → Borneo S
        gp(118,  5), gp(115, 20), gp(108, 22), gp(100, 22)
    });

    // ---- Japan ----
    m_regions["Japan"] = makePath({
        gp(130, 34), gp(130, 31), gp(131, 32), gp(132, 33),
        gp(135, 34), gp(138, 35), gp(141, 36),
        gp(141, 40), gp(143, 44), gp(145, 44),
        gp(141, 45), gp(141, 42), gp(140, 38), gp(139, 35), gp(136, 35)
    });

    // ---- Oceania (Australia) ----
    m_regions["Oceania"] = makePath({
        gp(114, -22), gp(114, -15), gp(122, -14), gp(130, -12),
        gp(136, -12), gp(139, -14), gp(143, -11),               // Cape York N tip
        gp(148, -20), gp(154, -22), gp(153, -28),
        gp(150, -38), gp(145, -38), gp(137, -35),
        gp(130, -32), gp(117, -35), gp(114, -28)
    });

    // ---- New Zealand ---- (North Island simplified)
    m_regions["New Zealand"] = makePath({
        gp(173, -34), gp(178, -37), gp(176, -39), gp(175, -41),
        gp(172, -40), gp(172, -36)
    });

    // ---- Greenland ----
    m_regions["Greenland"] = makePath({
        gp(-73, 76), gp(-50, 83), gp(-25, 83), gp(-18, 77),
        gp(-22, 70), gp(-45, 60), gp(-65, 63)
    });

    // Initialize highlight intensity for all regions to 0
    for (const QString& key : m_regions.keys())
        m_regionHighlights[key] = 0.0;
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

    // Deep cyber background (ocean color — shows through transparent SVG ocean areas)
    painter.fillRect(rect(), QColor("#090C10"));

    painter.save();
    // Scale local 1000x600 coordinates to fit the actual widget size
    qreal scaleX = width() / 1000.0;
    qreal scaleY = height() / 600.0;
    painter.scale(scaleX, scaleY);

    // --- Layer 1: SVG world map (detailed coastlines, pre-rendered) ---
    if (!m_mapPixmap.isNull())
        painter.drawPixmap(0, 0, m_mapPixmap);

    // --- Layer 2: subtle dot grid for the cyber aesthetic ---
    QPen gridPen(QColor(30, 40, 60, 60));
    gridPen.setWidthF(1.0);
    gridPen.setStyle(Qt::DotLine);
    painter.setPen(gridPen);
    for (int x = 0; x <= 1000; x += 50) painter.drawLine(x, 0, x, 600);
    for (int y = 0; y <= 600; y += 50) painter.drawLine(0, y, 1000, y);

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
            int r = static_cast<int>(0   * highlight);
            int g = static_cast<int>(255 * highlight);
            int b = static_cast<int>(255 * highlight);
            QColor fillColor(r, g, b, static_cast<int>(120 * highlight));
            QColor edgeColor(0, 255, 255, static_cast<int>(255 * highlight));

            painter.setBrush(fillColor);
            QPen regionPen(edgeColor);
            regionPen.setWidthF(2.5);
            painter.setPen(regionPen);
            painter.drawPath(path);

            // Label appears bright white when the region is active
            painter.setPen(QColor(255, 255, 255, static_cast<int>(230 * highlight)));
            QPointF center = path.boundingRect().center();
            painter.drawText(QRectF(center.x() - 70, center.y() - 10, 140, 20), Qt::AlignCenter, regionName);
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

#include "MapView.h"

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
    buildMapRegions();

    // Phone location: Oviedo, Asturias, Spain (lon=-5.845°, lat=43.361°).
    // Uses the same SVG-aligned projection as buildMapRegions().
    m_phonePos = svgCoord(-5.845, 43.361);

    // Service → region dictionary.
    // Each entry answers: "where does this traffic physically go from Spain?"
    // Regions must match keys in m_regions (built in buildMapRegions).

    // ── North America ── (US tech headquarters)
    m_serviceToRegion["SEARCH"]    = "North America";   // Google — Mountain View CA
    m_serviceToRegion["APPLE"]     = "North America";   // Apple — Cupertino CA
    m_serviceToRegion["SOCIAL"]    = "North America";   // Facebook/Instagram — Menlo Park CA
    m_serviceToRegion["MAPS"]      = "North America";   // Google Maps servers

    // ── Northern Europe ── (Meta EU / Microsoft Azure EU datacentres)
    m_serviceToRegion["WHATSAPP"]  = "Northern Europe"; // Meta EU — Netherlands/Denmark
    m_serviceToRegion["MICROSOFT"] = "Northern Europe"; // Azure EU West — Dublin / Amsterdam

    // ── Western Europe ── (AWS EU, EU financial & media infrastructure)
    m_serviceToRegion["AMAZON"]    = "Western Europe";  // AWS eu-west-1 — Ireland/Frankfurt
    m_serviceToRegion["EMAIL"]     = "Western Europe";  // Gmail EU, Outlook EU
    m_serviceToRegion["BANKING"]   = "Western Europe";  // EU banking — Frankfurt/Amsterdam
    m_serviceToRegion["NEWS"]      = "Western Europe";  // BBC, Reuters, El País
    m_serviceToRegion["CDN"]       = "Western Europe";  // Cloudflare EU PoPs
    m_serviceToRegion["SHOPPING"]  = "Western Europe";  // EU e-commerce

    // ── East Asia ── (streaming CDN, gaming, TikTok APAC)
    m_serviceToRegion["VIDEO"]     = "East Asia";       // YouTube APAC / TikTok CDN
    m_serviceToRegion["STREAMING"] = "East Asia";       // Netflix/Spotify APAC
    m_serviceToRegion["GAMING"]    = "East Asia";       // Steam Asia / PlayStation

    // ── Russia / CIS ── (VPN exit nodes — dramatic routing reveal)
    m_serviceToRegion["VPN"]       = "Russia / CIS";

    // ── South Asia ── (Indian tech-services, app analytics)
    m_serviceToRegion["TELEMETRY"] = "South Asia";      // Crashlytics, Firebase India nodes

    // ── Middle East ── (regional news, Islamic banking)
    m_serviceToRegion["REGIONAL"]  = "Middle East";

    // ── South America ── (WhatsApp Latin backbone, local social)
    m_serviceToRegion["LATAM"]     = "South America";

    // ── Oceania ── (antipodal CDN edge)
    m_serviceToRegion["PACIFIC"]   = "Oceania";
    
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

    // Override hardcoded polygons with saved editor data if regions.json exists.
    tryLoadRegionsFromFile();

    // Load dynamic service mapping from services.json
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

    // Clear and reload (replaces hardcoded ones)
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

void MapView::buildMapRegions()
{
    // All region vertices use svgCoord(lon, lat) so highlights align with the SVG map.
    // Coordinates calibrated from computed virtual-space positions (1000×600).
    // Only regions that appear in m_serviceToRegion are defined — no dead polygons.
    auto gp = [](qreal lon, qreal lat) -> QPointF {
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

   // ---- North America ----  [19 nodes]
m_regions["North America"] = makePath({
    gp( -145.0,   61.8),  // (  65,   92)
    gp( -155.1,   51.9),  // (  36,  132)
    gp( -121.9,   55.4),  // ( 129,  118)
    gp( -125.2,   29.4),  // ( 119,  223)
    gp( -103.2,   11.9),  // ( 180,  294)
    gp(  -88.4,   19.0),  // ( 222,  265)
    gp(  -91.4,   23.0),  // ( 213,  249)
    gp( -100.0,   21.4),  // ( 189,  255)
    gp(  -97.2,   26.5),  // ( 197,  235)
    gp(  -88.4,   28.5),  // ( 222,  227)
    gp(  -84.3,   24.1),  // ( 233,  244)
    gp(  -80.5,   31.1),  // ( 244,  216)
    gp(  -72.0,   39.3),  // ( 267,  183)
    gp(  -61.0,   43.2),  // ( 298,  167)
    gp(  -46.5,   47.2),  // ( 338,  151)
    gp(  -48.3,   59.7),  // ( 333,  100)
    gp(  -66.0,   67.6),  // ( 284,   68)
    gp( -100.0,   72.0),  // ( 189,   51)
    gp( -125.8,   72.1),  // ( 118,   50)
});

// ---- South America ----  [18 nodes]
m_regions["South America"] = makePath({
    gp(  -88.4,   19.1),  // ( 222,  264)
    gp(  -91.1,   23.0),  // ( 214,  249)
    gp(  -84.4,   24.2),  // ( 233,  244)
    gp(  -81.4,   28.5),  // ( 241,  226)
    gp(  -63.3,   17.5),  // ( 291,  271)
    gp(  -60.1,    8.0),  // ( 300,  310)
    gp(  -35.0,   -8.0),  // ( 370,  374)
    gp(  -41.8,  -24.8),  // ( 351,  442)
    gp(  -51.5,  -37.0),  // ( 324,  491)
    gp(  -59.4,  -48.7),  // ( 302,  539)
    gp(  -53.9,  -58.5),  // ( 317,  578)
    gp(  -62.4,  -58.5),  // ( 294,  578)
    gp(  -72.3,  -50.9),  // ( 266,  548)
    gp(  -75.0,  -34.2),  // ( 259,  480)
    gp(  -76.7,  -21.3),  // ( 254,  428)
    gp(  -89.4,   -8.1),  // ( 219,  375)
    gp(  -87.3,    4.8),  // ( 225,  323)
    gp( -102.9,   11.9),  // ( 181,  294)
});

// ---- Western Europe ----  [7 nodes]
m_regions["Western Europe"] = makePath({
    gp(   -7.3,   48.4),  // ( 447,  146)
    gp(   10.0,   54.0),  // ( 495,  123)
    gp(   16.0,   48.0),  // ( 512,  148)
    gp(    9.0,   45.0),  // ( 492,  160)
    gp(    3.9,   38.1),  // ( 478,  188)
    gp(   -5.0,   36.0),  // ( 453,  196)
    gp(   -9.0,   38.0),  // ( 442,  188)
});

// ---- Northern Europe ----  [8 nodes]
m_regions["Northern Europe"] = makePath({
    gp(    9.8,   54.5),  // ( 495,  122)
    gp(   19.4,   55.5),  // ( 521,  117)
    gp(   29.3,   56.9),  // ( 548,  112)
    gp(   33.0,   69.0),  // ( 559,   63)
    gp(   25.3,   72.3),  // ( 538,   49)
    gp(   16.1,   70.3),  // ( 512,   58)
    gp(    5.6,   62.8),  // ( 483,   88)
    gp(    7.9,   57.1),  // ( 489,  111)
});

// ---- Russia / CIS ----  [14 nodes]
m_regions["Russia / CIS"] = makePath({
    gp(   29.6,   56.3),  // ( 549,  114)
    gp(   34.0,   52.4),  // ( 562,  130)
    gp(   49.0,   46.0),  // ( 603,  156)
    gp(   77.2,   39.7),  // ( 682,  181)
    gp(   85.6,   49.1),  // ( 705,  143)
    gp(  106.5,   52.7),  // ( 763,  129)
    gp(  121.5,   54.7),  // ( 805,  121)
    gp(  139.4,   45.2),  // ( 855,  159)
    gp(  163.0,   53.0),  // ( 920,  128)
    gp(  160.5,   67.4),  // ( 913,   70)
    gp(  130.0,   72.0),  // ( 828,   51)
    gp(   77.1,   77.2),  // ( 681,   30)
    gp(   54.2,   72.6),  // ( 618,   48)
    gp(   38.8,   68.3),  // ( 575,   66)
});

// ---- Africa ----  [16 nodes]
m_regions["Africa"] = makePath({
    gp(  -16.0,   28.0),  // ( 423,  229)
    gp(   -4.0,   36.1),  // ( 456,  196)
    gp(    4.7,   38.1),  // ( 480,  188)
    gp(   36.9,   31.8),  // ( 570,  213)
    gp(   42.6,   17.1),  // ( 586,  273)
    gp(   48.8,    9.9),  // ( 603,  302)
    gp(   56.7,   11.5),  // ( 625,  295)
    gp(   46.1,   -4.4),  // ( 595,  359)
    gp(   59.0,  -17.5),  // ( 631,  413)
    gp(   50.8,  -29.6),  // ( 608,  462)
    gp(   20.6,  -38.6),  // ( 524,  498)
    gp(   12.1,  -23.0),  // ( 501,  435)
    gp(   13.0,   -9.0),  // ( 503,  378)
    gp(    5.4,    3.6),  // ( 482,  327)
    gp(   -7.9,    1.3),  // ( 445,  337)
    gp(  -19.8,   13.0),  // ( 412,  289)
});

// ---- Middle East ----  [11 nodes]
m_regions["Middle East"] = makePath({
    gp(   27.3,   41.6),  // ( 543,  174)
    gp(   38.4,   43.0),  // ( 574,  168)
    gp(   45.5,   42.4),  // ( 594,  170)
    gp(   47.3,   38.1),  // ( 599,  188)
    gp(   54.9,   31.0),  // ( 620,  217)
    gp(   65.8,   22.9),  // ( 650,  249)
    gp(   63.3,   17.3),  // ( 643,  272)
    gp(   47.9,   11.5),  // ( 600,  295)
    gp(   37.5,   31.5),  // ( 571,  214)
    gp(   33.7,   35.1),  // ( 561,  200)
    gp(   29.1,   36.6),  // ( 548,  194)
});

// ---- South Asia ----  [12 nodes]
m_regions["South Asia"] = makePath({
    gp(   54.6,   31.6),  // ( 619,  214)
    gp(   45.5,   42.4),  // ( 594,  170)
    gp(   77.5,   38.9),  // ( 683,  185)
    gp(  103.3,   30.0),  // ( 754,  220)
    gp(  108.9,   20.5),  // ( 770,  259)
    gp(  103.5,   13.5),  // ( 755,  287)
    gp(   98.6,   20.1),  // ( 741,  261)
    gp(   89.8,   14.8),  // ( 717,  282)
    gp(   91.9,    6.7),  // ( 723,  315)
    gp(   87.5,    2.0),  // ( 710,  334)
    gp(   73.0,   19.0),  // ( 670,  265)
    gp(   65.4,   23.3),  // ( 649,  248)
});

// ---- East Asia ----  [15 nodes]
m_regions["East Asia"] = makePath({
    gp(   86.5,   49.2),  // ( 707,  143)
    gp(  121.5,   54.3),  // ( 805,  122)
    gp(  139.7,   45.9),  // ( 855,  156)
    gp(  147.9,   35.3),  // ( 878,  199)
    gp(  131.1,   29.8),  // ( 831,  222)
    gp(  123.5,   20.5),  // ( 810,  259)
    gp(  132.5,   19.4),  // ( 835,  263)
    gp(  141.3,    6.4),  // ( 860,  316)
    gp(  114.4,   -8.8),  // ( 785,  377)
    gp(  102.7,    4.4),  // ( 753,  324)
    gp(  107.3,    8.3),  // ( 765,  308)
    gp(  103.5,   13.8),  // ( 755,  286)
    gp(  109.1,   20.9),  // ( 770,  257)
    gp(  104.7,   29.2),  // ( 758,  224)
    gp(   76.6,   39.6),  // ( 680,  182)
});

// ---- Oceania ----  [13 nodes]
m_regions["Oceania"] = makePath({
    gp(  119.3,  -37.9),  // ( 798,  495)
    gp(  120.2,  -23.3),  // ( 801,  436)
    gp(  132.3,  -14.6),  // ( 835,  401)
    gp(  114.7,   -9.1),  // ( 786,  378)
    gp(  141.1,    6.0),  // ( 859,  318)
    gp(  181.5,   -9.1),  // ( 971,  378)
    gp(  172.5,  -15.9),  // ( 946,  406)
    gp(  157.5,  -11.9),  // ( 905,  390)
    gp(  165.5,  -28.7),  // ( 927,  458)
    gp(  186.9,  -41.1),  // ( 987,  508)
    gp(  165.1,  -52.3),  // ( 926,  553)
    gp(  145.5,  -47.2),  // ( 871,  533)
    gp(  137.6,  -37.0),  // ( 849,  491)
});

    // ---- Mediterranean (Italy + Greece + Balkans) ----
    // Assign a service in m_serviceToRegion to activate this region.
    m_regions["Mediterranean"] = makePath({
        gp(  9.0,  44.5),  // N Italy
        gp( 13.5,  45.5),  // NE Italy / Trieste
        gp( 18.5,  40.5),  // Italian heel
        gp( 16.0,  38.0),  // Italian toe
        gp( 12.5,  37.0),  // Sicily W
        gp( 14.5,  37.0),  // Sicily E
        gp( 23.0,  35.0),  // Crete
        gp( 27.0,  36.0),  // Rhodes
        gp( 26.0,  41.0),  // E Greece / N Aegean
        gp( 22.0,  41.0),  // N Greece
        gp( 19.0,  42.0),  // Albania coast
        gp( 14.5,  44.0),  // Croatia / Dalmatia
    });

    // Initialize highlight intensity for all regions to 0
    for (const QString& key : m_regions.keys())
        m_regionHighlights[key] = 0.0;
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
            m_regionHighlights[m_connections[i].targetRegion] = 1.0;
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

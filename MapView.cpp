#include "MapView.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QColor>
#include <QTimer>
#include <QRandomGenerator>
#include <QFont>

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    // Define locations on a conceptual 1000x600 grid
    // These represent generalized geographic/cloud regions
    m_serviceLocations["SEARCH"]    = QPointF(200, 200); // Silicon Valley / US West
    m_serviceLocations["APPLE"]     = QPointF(150, 250); // Cupertino / US West
    m_serviceLocations["MICROSOFT"] = QPointF(250, 150); // Redmond / US NW
    m_serviceLocations["AMAZON"]    = QPointF(750, 200); // Frankfurt / EU
    m_serviceLocations["WHATSAPP"]  = QPointF(400, 450); // General LatAm/Global
    m_serviceLocations["VIDEO"]     = QPointF(850, 300); // CDNs in Asia/EU

    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &MapView::updateAnimations);
    m_animationTimer->start(30); // ~33 FPS
}

void MapView::addConnection(const QString& eventType)
{
    MapConnection conn;
    // Fallback to "SEARCH" destination if service is unknown, or pick a random spot
    conn.service = eventType.isEmpty() ? "UNKNOWN" : eventType;
    conn.progress = 0.0;
    m_connections.append(conn);
}

void MapView::updateAnimations()
{
    m_pulsePhase = (m_pulsePhase + 1) % 40;

    for (int i = m_connections.size() - 1; i >= 0; --i) {
        m_connections[i].progress += 0.015; // Speed of animation
        if (m_connections[i].progress >= 1.0) {
            m_connections.removeAt(i);
        }
    }
    
    // Always update to keep the phone node pulsing
    update();
}

void MapView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Deep cyber background
    painter.fillRect(rect(), QColor("#090C10"));

    // Scale mapping to fit the widget exactly
    painter.save();
    qreal scaleX = width() / 1000.0;
    qreal scaleY = height() / 600.0;
    painter.scale(scaleX, scaleY);

    // Draw stylized grid map
    QPen gridPen(QColor(30, 40, 60, 150));
    gridPen.setWidthF(1.0);
    gridPen.setStyle(Qt::DotLine);
    painter.setPen(gridPen);
    for (int x = 0; x <= 1000; x += 50) {
        painter.drawLine(x, 0, x, 600);
    }
    for (int y = 0; y <= 600; y += 50) {
        painter.drawLine(0, y, 1000, y);
    }

    // Phone location (Center)
    QPointF phonePos(500, 300);
    
    // Draw known service nodes
    for (auto it = m_serviceLocations.begin(); it != m_serviceLocations.end(); ++it) {
        painter.setBrush(QColor("#0077FF")); // Deep Blue
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(it.value(), 6, 6);
        
        painter.setPen(QColor(150, 180, 220));
        QFont font("Consolas");
        font.setPixelSize(14);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(it.value() + QPointF(12, 5), it.key());
    }

    // Draw active animated connections
    for (const auto& conn : m_connections) {
        QPointF endPos = m_serviceLocations.value(conn.service, QPointF(0, 0));
        
        // Random fallback target if unknown service
        if (endPos.isNull()) {
            endPos = QPointF(
                (qHash(conn.service) % 800) + 100, 
                ((qHash(conn.service) / 10) % 400) + 100
            );
            // Draw a temporary node for this unknown service
            painter.setBrush(QColor("#888888"));
            painter.drawEllipse(endPos, 4, 4);
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(endPos + QPointF(10, 5), conn.service);
        }

        // Animated fading line
        QPen linePen(QColor(0, 255, 200, 80)); // Cyan, semi-transparent
        linePen.setWidthF(2.0);
        painter.setPen(linePen);
        painter.drawLine(phonePos, endPos);

        // Moving data packet
        QPointF currentPos = phonePos + (endPos - phonePos) * conn.progress;
        
        // Inner core
        painter.setBrush(QColor("#FFFFFF"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(currentPos, 4, 4);
        
        // Outer glow
        painter.setBrush(QColor(0, 255, 255, 120));
        painter.drawEllipse(currentPos, 8, 8);
    }

    // Draw phone node (Center point)
    painter.setBrush(QColor("#00FF44")); // Cyber Green
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(phonePos, 8, 8);
    
    // Draw animated pulse around the phone
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(0, 255, 68, 150 - m_pulsePhase * 3), 2.0));
    painter.drawEllipse(phonePos, 8 + m_pulsePhase / 1.5, 8 + m_pulsePhase / 1.5);
    
    painter.setPen(QColor(200, 255, 200));
    QFont font("Consolas");
    font.setPixelSize(16);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(phonePos + QPointF(-40, 30), "TARGET PHONE");

    painter.restore();
}

// Визуальное соединение — кривая Безье, цвет по типу данных
#include "ConnectionItem.h"
#include "PortItem.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <QPen>
#include <cmath>

namespace DeltaQ {

ConnectionItem::ConnectionItem(PortItem *sourcePort, PortItem *destPort,
                               QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
    , m_sourcePort(sourcePort)
    , m_destPort(destPort)
{
    setZValue(-1); // За узлами
    setFlag(ItemIsSelectable);
    m_dataType = sourcePort ? sourcePort->portType() : QString();
    updatePath();
}

ConnectionItem::ConnectionItem(PortItem *sourcePort, QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
    , m_sourcePort(sourcePort)
{
    setZValue(-1);
    setFlag(ItemIsSelectable);
    m_dataType = sourcePort ? sourcePort->portType() : QString();
}

void ConnectionItem::setDestPort(PortItem *port)
{
    m_destPort = port;
    updatePath();
}

void ConnectionItem::setTempEndPoint(const QPointF &point)
{
    m_tempEndPoint = point;
    computeBezierPath();
}

void ConnectionItem::updatePath()
{
    computeBezierPath();
}

void ConnectionItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

QPainterPath ConnectionItem::shape() const
{
    QPainterPathStroker stroker;
    stroker.setWidth(10.0); // Расширенная зона клика
    return stroker.createStroke(path());
}

void ConnectionItem::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor color = PortItem::colorForType(m_dataType);
    bool selected = isSelected();
    qreal width = (m_highlighted || selected) ? 3.0 : 2.0;
    if (selected)
        color = QColor(255, 100, 100); // красный при выделении

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(color, width));
    painter->drawPath(path());
}

void ConnectionItem::computeBezierPath()
{
    QPointF start;
    QPointF end;

    if (m_sourcePort)
        start = m_sourcePort->centerInScene();

    if (m_destPort)
        end = m_destPort->centerInScene();
    else
        end = m_tempEndPoint;

    // Расстояние для control points (50-150px)
    qreal dx = std::abs(end.x() - start.x());
    qreal offset = qBound(50.0, dx * 0.5, 150.0);

    QPainterPath p;
    p.moveTo(start);
    p.cubicTo(start + QPointF(offset, 0),
              end + QPointF(-offset, 0),
              end);

    setPath(p);
}

} // namespace DeltaQ

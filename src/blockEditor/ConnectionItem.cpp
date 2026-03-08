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
    m_kind = sourcePort ? sourcePort->portKind() : PortKind::Data;
    updatePath();
}

ConnectionItem::ConnectionItem(PortItem *sourcePort, QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
    , m_sourcePort(sourcePort)
{
    setZValue(-1);
    setFlag(ItemIsSelectable);
    m_dataType = sourcePort ? sourcePort->portType() : QString();
    m_kind = sourcePort ? sourcePort->portKind() : PortKind::Data;
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

    bool selected = isSelected();

    if (m_kind == PortKind::Execution) {
        // Execution-связи: белая линия 2.5px
        QColor color(220, 220, 220);
        qreal width = (m_highlighted || selected) ? 3.5 : 2.5;
        if (selected)
            color = QColor(255, 100, 100);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(color, width));
        painter->drawPath(path());
    } else {
        // Data-связи: цвет по типу
        QColor color = PortItem::colorForType(m_dataType);
        qreal width = (m_highlighted || selected) ? 3.0 : 2.0;
        if (selected)
            color = QColor(255, 100, 100);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(color, width));
        painter->drawPath(path());
    }
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

    QPainterPath p;
    p.moveTo(start);

    if (m_kind == PortKind::Execution) {
        const qreal dy = std::abs(end.y() - start.y());
        const qreal offset = qBound(50.0, dy * 0.5, 150.0);
        p.cubicTo(start + QPointF(0, offset),
                  end + QPointF(0, -offset),
                  end);
    } else {
        const qreal dx = std::abs(end.x() - start.x());
        const qreal offset = qBound(50.0, dx * 0.5, 150.0);
        p.cubicTo(start + QPointF(offset, 0),
                  end + QPointF(-offset, 0),
                  end);
    }

    setPath(p);
}

} // namespace DeltaQ

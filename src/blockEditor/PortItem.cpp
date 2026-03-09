// Визуальный порт узла — отрисовка, hover-эффект, цвет по типу данных
#include "PortItem.h"
#include "NodeItem.h"
#include "ConnectionItem.h"

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QPen>
#include <QFont>

namespace DeltaQ {

PortItem::PortItem(const QString &name, const QString &type,
                   PortDirection direction, PortKind kind, NodeItem *parent)
    : QGraphicsItem(parent)
    , m_name(name)
    , m_type(type)
    , m_direction(direction)
    , m_kind(kind)
    , m_parentNode(parent)
{
    setAcceptHoverEvents(true);
    setToolTip(QString("%1 (%2)").arg(m_name, m_type));

    // Подпись порта
    m_label = new QGraphicsTextItem(this);
    m_label->setDefaultTextColor(Qt::white);
    QFont font("Sans", 8);
    m_label->setFont(font);
    if (m_kind == PortKind::Execution)
        m_label->setPlainText(m_name);
    else
        m_label->setPlainText(QString("%1 (%2)").arg(m_name, m_type));

    updateAppearance();
}

QRectF PortItem::boundingRect() const
{
    qreal r = m_hovered ? HoverRadius : NormalRadius;
    return QRectF(-r, -r, r * 2, r * 2);
}

void PortItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);
    qreal r = m_hovered ? HoverRadius : NormalRadius;

    if (m_kind == PortKind::Execution) {
        // Треугольная форма для exec-портов
        QColor color(220, 220, 220); // белый
        painter->setBrush(color);
        painter->setPen(QPen(color.darker(130), 1));

        QPolygonF triangle;
        if (m_direction == PortDirection::Output) {
            triangle << QPointF(-r, -r)
                     << QPointF(r, 0)
                     << QPointF(-r, r);
        } else {
            triangle << QPointF(r, -r)
                     << QPointF(-r, 0)
                     << QPointF(r, r);
        }
        painter->drawPolygon(triangle);
    } else {
        // Круглая форма для data-портов
        QColor color = colorForType(m_type);
        painter->setBrush(color);
        painter->setPen(QPen(color.darker(130), 1));
        painter->drawEllipse(QRectF(-r, -r, r * 2, r * 2));
    }
}

QPainterPath PortItem::shape() const
{
    QPainterPath path;
    qreal r = m_hovered ? HoverRadius : NormalRadius;
    if (m_kind == PortKind::Execution) {
        QPolygonF triangle;
        if (m_direction == PortDirection::Output) {
            triangle << QPointF(-r, -r) << QPointF(r, 0) << QPointF(-r, r);
        } else {
            triangle << QPointF(r, -r) << QPointF(-r, 0) << QPointF(r, r);
        }
        path.addPolygon(triangle);
        path.closeSubpath();
    } else {
        path.addEllipse(QRectF(-r, -r, r * 2, r * 2));
    }
    return path;
}

QPointF PortItem::centerInScene() const
{
    return mapToScene(QPointF(0, 0));
}

QColor PortItem::colorForType(const QString &type)
{
    if (type == "int")    return QColor(100, 180, 255); // голубой
    if (type == "float")  return QColor(100, 220, 100); // зелёный
    if (type == "double") return QColor(100, 220, 100); // зелёный
    if (type == "bool")   return QColor(255, 180, 80);  // оранжевый
    if (type == "string") return QColor(180, 120, 255); // фиолетовый
    if (type == "exec")   return QColor(220, 220, 220); // белый
    return QColor(180, 180, 180); // серый по умолчанию
}

QSizeF PortItem::labelSize() const
{
    return m_label ? m_label->boundingRect().size() : QSizeF();
}

QRectF PortItem::labelRectInNode() const
{
    if (!m_label || !m_parentNode)
        return {};
    return m_label->mapRectToItem(m_parentNode, m_label->boundingRect());
}

void PortItem::addConnection(ConnectionItem *conn)
{
    if (!m_connections.contains(conn))
        m_connections.append(conn);
}

void PortItem::removeConnection(ConnectionItem *conn)
{
    m_connections.removeAll(conn);
}

void PortItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    prepareGeometryChange();
    updateAppearance();
    update();
}

void PortItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    prepareGeometryChange();
    updateAppearance();
    update();
}

void PortItem::updateAppearance()
{
    qreal r = m_hovered ? HoverRadius : NormalRadius;

    if (m_label) {
        qreal tw = m_label->boundingRect().width();
        qreal th = m_label->boundingRect().height();
        if (m_kind == PortKind::Execution) {
            if (m_direction == PortDirection::Input)
                m_label->setPos(r + 6, -th / 2);
            else
                m_label->setPos(-r - 6 - tw, -th / 2);
            return;
        }

        // Data-подписи: input справа, output слева
        if (m_direction == PortDirection::Input)
            m_label->setPos(r + 4, -th / 2);
        else
            m_label->setPos(-r - 4 - tw, -th / 2);
    }
}

} // namespace DeltaQ

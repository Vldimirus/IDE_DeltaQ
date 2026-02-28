// Визуальный порт узла — отрисовка, hover-эффект, цвет по типу данных
#include "PortItem.h"
#include "NodeItem.h"
#include "ConnectionItem.h"

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include <QFont>

namespace DeltaQ {

PortItem::PortItem(const QString &name, const QString &type,
                   PortDirection direction, NodeItem *parent)
    : QGraphicsEllipseItem(parent)
    , m_name(name)
    , m_type(type)
    , m_direction(direction)
    , m_parentNode(parent)
{
    setAcceptHoverEvents(true);
    setToolTip(QString("%1 (%2)").arg(m_name, m_type));

    // Подпись порта
    m_label = new QGraphicsTextItem(this);
    m_label->setDefaultTextColor(Qt::white);
    QFont font("Sans", 8);
    m_label->setFont(font);
    m_label->setPlainText(QString("%1 (%2)").arg(m_name, m_type));

    updateAppearance();
}

QPointF PortItem::centerInScene() const
{
    return mapToScene(rect().center());
}

QColor PortItem::colorForType(const QString &type)
{
    if (type == "int")    return QColor(100, 180, 255); // голубой
    if (type == "float")  return QColor(100, 220, 100); // зелёный
    if (type == "double") return QColor(100, 220, 100); // зелёный
    if (type == "bool")   return QColor(255, 180, 80);  // оранжевый
    if (type == "string") return QColor(180, 120, 255); // фиолетовый
    return QColor(180, 180, 180); // серый по умолчанию
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
    qreal r = HoverRadius;
    setRect(-r, -r, r * 2, r * 2);
    update();
}

void PortItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    updateAppearance();
}

void PortItem::updateAppearance()
{
    qreal r = NormalRadius;
    setRect(-r, -r, r * 2, r * 2);

    QColor color = colorForType(m_type);
    setBrush(color);
    setPen(QPen(color.darker(130), 1));

    // Позиция подписи: input — справа, output — слева
    if (m_label) {
        qreal tw = m_label->boundingRect().width();
        qreal th = m_label->boundingRect().height();
        if (m_direction == PortDirection::Input)
            m_label->setPos(r + 4, -th / 2);
        else
            m_label->setPos(-r - 4 - tw, -th / 2);
    }
}

} // namespace DeltaQ

// Визуальный узел графа — отрисовка, порты, состояния
#include "NodeItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>

namespace DeltaQ {

NodeItem::NodeItem(const QString &nodeId, const QString &moduleName,
                   const QString &category, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_nodeId(nodeId)
    , m_moduleName(moduleName)
    , m_category(category)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
}

void NodeItem::addInputPort(const QString &name, const QString &type,
                            PortKind kind)
{
    auto *port = new PortItem(name, type, PortDirection::Input, kind, this);
    m_inputPorts.append(port);
    updatePortPositions();
}

void NodeItem::addOutputPort(const QString &name, const QString &type,
                             PortKind kind)
{
    auto *port = new PortItem(name, type, PortDirection::Output, kind, this);
    m_outputPorts.append(port);
    updatePortPositions();
}

PortItem *NodeItem::findPort(const QString &name, PortDirection dir) const
{
    const auto &ports = (dir == PortDirection::Input) ? m_inputPorts : m_outputPorts;
    for (auto *p : ports)
        if (p->portName() == name) return p;
    return nullptr;
}

QRectF NodeItem::boundingRect() const
{
    // Считаем data-порты отдельно (exec-порты размещаются в заголовке)
    int maxDataPorts = 0;
    for (auto *p : m_inputPorts)
        if (p->portKind() == PortKind::Data) maxDataPorts++;
    int outDataPorts = 0;
    for (auto *p : m_outputPorts)
        if (p->portKind() == PortKind::Data) outDataPorts++;
    maxDataPorts = qMax(maxDataPorts, outDataPorts);

    qreal h = HeaderHeight + maxDataPorts * PortSpacing + BottomPadding;
    return QRectF(0, 0, Width, h);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                     QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF r = boundingRect();

    // Тело узла
    painter->setBrush(QColor(45, 45, 45)); // #2d2d2d

    // Цвет рамки по состоянию
    QColor borderColor(100, 100, 100); // обычная серая
    qreal borderWidth = 1.5;

    if (m_hasError) {
        borderColor = QColor(220, 60, 60);   // красный
        borderWidth = 2.5;
    } else if (m_highlighted) {
        borderColor = QColor(255, 220, 50);   // жёлтый
        borderWidth = 2.5;
    } else if (m_completed) {
        borderColor = QColor(80, 200, 80);    // зелёный
        borderWidth = 2.0;
    } else if (isSelected()) {
        borderColor = QColor(70, 130, 220);   // синий
        borderWidth = 2.0;
    }

    painter->setPen(QPen(borderColor, borderWidth));
    painter->drawRoundedRect(r, 6, 6);

    // Заголовок
    QRectF headerRect(0, 0, Width, HeaderHeight);
    painter->setBrush(categoryColor(m_category));
    painter->setPen(Qt::NoPen);

    // Рисуем заголовок с закруглением только сверху
    QPainterPath headerPath;
    headerPath.moveTo(6, HeaderHeight);
    headerPath.lineTo(0, HeaderHeight);         // нижний левый угол (прямой)
    headerPath.lineTo(0, 6);
    headerPath.arcTo(0, 0, 12, 12, 180, -90);  // верхний левый (скруглённый)
    headerPath.lineTo(Width - 6, 0);
    headerPath.arcTo(Width - 12, 0, 12, 12, 90, -90); // верхний правый (скруглённый)
    headerPath.lineTo(Width, HeaderHeight);     // нижний правый угол (прямой)
    headerPath.closeSubpath();
    painter->drawPath(headerPath);

    // Текст заголовка
    painter->setPen(Qt::white);
    QFont headerFont("Sans", 10, QFont::Bold);
    painter->setFont(headerFont);
    painter->drawText(headerRect, Qt::AlignCenter, m_moduleName);

    // Подсказка при ошибке
    if (m_hasError && !m_errorMessage.isEmpty())
        setToolTip(m_errorMessage);
}

QColor NodeItem::categoryColor(const QString &category)
{
    if (category == "math")   return QColor(50, 100, 180);  // синий
    if (category == "logic")  return QColor(50, 140, 80);   // зелёный
    if (category == "io")     return QColor(200, 120, 40);  // оранжевый
    if (category == "string") return QColor(140, 80, 180);  // фиолетовый
    return QColor(100, 100, 100); // серый для custom
}

void NodeItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

void NodeItem::setCompleted(bool on)
{
    m_completed = on;
    update();
}

void NodeItem::setError(bool on, const QString &message)
{
    m_hasError = on;
    m_errorMessage = message;
    if (!on)
        setToolTip({});
    update();
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        // Обновляем все соединения при перемещении узла
        for (auto *port : m_inputPorts)
            for (auto *conn : port->connections())
                conn->updatePath();
        for (auto *port : m_outputPorts)
            for (auto *conn : port->connections())
                conn->updatePath();
    }
    return QGraphicsObject::itemChange(change, value);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragStartPos = pos();
    QGraphicsObject::mousePressEvent(event);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsObject::mouseReleaseEvent(event);

    // Если позиция реально изменилась — сигнализируем для MoveCommand
    if (pos() != m_dragStartPos)
        emit positionChanged(m_nodeId, pos());
}

void NodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    emit doubleClicked(m_nodeId);
}

void NodeItem::updatePortPositions()
{
    // Exec-порты — в области заголовка (по бокам)
    // Data-порты — ниже заголовка (как раньше)
    int execInIdx = 0, execOutIdx = 0;
    int dataInIdx = 0, dataOutIdx = 0;

    for (auto *port : m_inputPorts) {
        if (port->portKind() == PortKind::Execution) {
            // Exec input — слева в области заголовка
            qreal y = HeaderHeight * 0.5;
            qreal x = -2.0; // чуть левее края
            Q_UNUSED(execInIdx)
            port->setPos(x, y);
            execInIdx++;
        } else {
            qreal y = HeaderHeight + PortSpacing * 0.5 + dataInIdx * PortSpacing;
            port->setPos(0, y);
            dataInIdx++;
        }
    }

    for (auto *port : m_outputPorts) {
        if (port->portKind() == PortKind::Execution) {
            // Exec output — справа в области заголовка
            qreal y = HeaderHeight * 0.5;
            qreal x = Width + 2.0; // чуть правее края
            Q_UNUSED(execOutIdx)
            port->setPos(x, y);
            execOutIdx++;
        } else {
            qreal y = HeaderHeight + PortSpacing * 0.5 + dataOutIdx * PortSpacing;
            port->setPos(Width, y);
            dataOutIdx++;
        }
    }

    // Обновить геометрию
    prepareGeometryChange();
}

} // namespace DeltaQ

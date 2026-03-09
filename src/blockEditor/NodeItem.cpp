// Визуальный узел графа — отрисовка, порты, состояния
#include "NodeItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"

#include <algorithm>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QFontMetricsF>

namespace DeltaQ {

namespace {

int executionPortCount(const QVector<PortItem *> &ports)
{
    int count = 0;
    for (auto *port : ports) {
        if (port->portKind() == PortKind::Execution)
            ++count;
    }
    return count;
}

} // namespace

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
    updatePortPositions();
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
    return QRectF(0, 0, m_width, m_height);
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
    QRectF headerRect(0, 0, m_width, m_headerHeight);
    painter->setBrush(categoryColor(m_category));
    painter->setPen(Qt::NoPen);

    // Рисуем заголовок с закруглением только сверху
    QPainterPath headerPath;
    headerPath.moveTo(6, m_headerHeight);
    headerPath.lineTo(0, m_headerHeight);         // нижний левый угол (прямой)
    headerPath.lineTo(0, 6);
    headerPath.arcTo(0, 0, 12, 12, 180, -90);  // верхний левый (скруглённый)
    headerPath.lineTo(m_width - 6, 0);
    headerPath.arcTo(m_width - 12, 0, 12, 12, 90, -90); // верхний правый (скруглённый)
    headerPath.lineTo(m_width, m_headerHeight);     // нижний правый угол (прямой)
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
    if (category == "desktop") return QColor(40, 150, 160); // бирюзовый
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
    const qreal newWidth = computeWidth();
    const qreal newHeaderHeight = computeHeaderHeight();
    const qreal newHeight = computeHeight();
    if (!qFuzzyCompare(m_width, newWidth)
        || !qFuzzyCompare(m_height, newHeight)
        || !qFuzzyCompare(m_headerHeight, newHeaderHeight)) {
        prepareGeometryChange();
        m_width = newWidth;
        m_headerHeight = newHeaderHeight;
        m_height = newHeight;
    }

    int execInIdx = 0;
    int execOutIdx = 0;
    int dataInIdx = 0;
    int dataOutIdx = 0;
    const int execInCount = executionPortCount(m_inputPorts);
    const int execOutCount = executionPortCount(m_outputPorts);

    const qreal inputExecSpan = execInCount > 0 ? (execInCount - 1) * ExecPortSpacing : 0.0;
    const qreal outputExecSpan = execOutCount > 0 ? (execOutCount - 1) * ExecPortSpacing : 0.0;
    const qreal inputExecStart = (m_headerHeight - inputExecSpan) * 0.5;
    const qreal outputExecStart = (m_headerHeight - outputExecSpan) * 0.5;

    for (auto *port : m_inputPorts) {
        if (port->portKind() == PortKind::Execution) {
            qreal x = ExecPortInset;
            qreal y = inputExecStart + execInIdx * ExecPortSpacing;
            port->setPos(x, y);
            execInIdx++;
        } else {
            qreal y = m_headerHeight + PortSpacing * 0.5 + dataInIdx * PortSpacing;
            port->setPos(0, y);
            dataInIdx++;
        }
    }

    for (auto *port : m_outputPorts) {
        if (port->portKind() == PortKind::Execution) {
            qreal x = m_width - ExecPortInset;
            qreal y = outputExecStart + execOutIdx * ExecPortSpacing;
            port->setPos(x, y);
            execOutIdx++;
        } else {
            qreal y = m_headerHeight + PortSpacing * 0.5 + dataOutIdx * PortSpacing;
            port->setPos(m_width, y);
            dataOutIdx++;
        }
    }

    update();
}

qreal NodeItem::computeWidth() const
{
    QFont headerFont("Sans", 10, QFont::Bold);
    QFontMetricsF headerMetrics(headerFont);
    qreal titleWidth = headerMetrics.horizontalAdvance(m_moduleName);

    qreal leftLabelWidth = 0.0;
    qreal rightLabelWidth = 0.0;
    int execInCount = 0;
    int execOutCount = 0;
    qreal execInLabelWidth = 0.0;
    qreal execOutLabelWidth = 0.0;

    for (auto *port : m_inputPorts) {
        if (port->portKind() == PortKind::Execution) {
            execInCount++;
            execInLabelWidth = qMax(execInLabelWidth, port->labelSize().width());
        } else {
            leftLabelWidth = qMax(leftLabelWidth, port->labelSize().width());
        }
    }

    for (auto *port : m_outputPorts) {
        if (port->portKind() == PortKind::Execution) {
            execOutCount++;
            execOutLabelWidth = qMax(execOutLabelWidth, port->labelSize().width());
        } else {
            rightLabelWidth = qMax(rightLabelWidth, port->labelSize().width());
        }
    }

    const qreal execMarkerWidth = 14.0;
    const qreal leftHeaderWidth = execInCount > 0
        ? ExecColumnPadding + ExecPortInset + execMarkerWidth + ExecLabelGap + execInLabelWidth
        : 0.0;
    const qreal rightHeaderWidth = execOutCount > 0
        ? ExecColumnPadding + ExecPortInset + execMarkerWidth + ExecLabelGap + execOutLabelWidth
        : 0.0;
    const qreal headerWidth = leftHeaderWidth + rightHeaderWidth
        + titleWidth + HeaderTitleGap * 2.0;

    const qreal dataWidth = leftLabelWidth + rightLabelWidth
        + SidePadding * 2.0 + PortLabelGap * 2.0 + CenterGap;

    return std::max({MinWidth, headerWidth, dataWidth});
}

qreal NodeItem::computeHeight() const
{
    int maxDataPorts = 0;
    for (auto *p : m_inputPorts)
        if (p->portKind() == PortKind::Data) maxDataPorts++;
    int outDataPorts = 0;
    for (auto *p : m_outputPorts)
        if (p->portKind() == PortKind::Data) outDataPorts++;
    maxDataPorts = qMax(maxDataPorts, outDataPorts);

    return computeHeaderHeight() + maxDataPorts * PortSpacing + BottomPadding;
}

qreal NodeItem::computeHeaderHeight() const
{
    const int execRows = qMax(executionPortCount(m_inputPorts), executionPortCount(m_outputPorts));
    if (execRows <= 1)
        return BaseHeaderHeight;

    return qMax(BaseHeaderHeight,
                ExecVerticalPadding * 2.0 + (execRows - 1) * ExecPortSpacing + 1.0);
}

} // namespace DeltaQ

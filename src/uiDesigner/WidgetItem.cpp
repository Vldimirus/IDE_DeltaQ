// Визуальный элемент UI-виджета — реализация
#include "WidgetItem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QtMath>

namespace DeltaQ {

WidgetItem::WidgetItem(const UIWidget &widget, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_widget(widget)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setPos(widget.geometry.x(), widget.geometry.y());
}

UIWidget WidgetItem::toUIWidget() const
{
    UIWidget w = m_widget;
    w.geometry.setX(pos().x());
    w.geometry.setY(pos().y());

    // Рекурсивно дочерние
    w.children.clear();
    for (auto *child : m_children)
        w.children.append(child->toUIWidget());

    return w;
}

void WidgetItem::fromUIWidget(const UIWidget &widget)
{
    m_widget = widget;
    setPos(widget.geometry.x(), widget.geometry.y());
    update();
}

QVariant WidgetItem::property(const QString &key) const
{
    return m_widget.properties.value(key);
}

void WidgetItem::setWidgetProperty(const QString &key, const QVariant &value)
{
    m_widget.properties[key] = value;
    update();
    emit propertyChanged(m_widget.id, key, value);
}

void WidgetItem::setWidgetSize(qreal w, qreal h)
{
    prepareGeometryChange();
    m_widget.geometry.setWidth(qMax(w, MinWidth));
    m_widget.geometry.setHeight(qMax(h, MinHeight));
    update();
}

void WidgetItem::setEvent(const QString &event, const QString &handler)
{
    m_widget.events[event] = handler;
}

void WidgetItem::removeEvent(const QString &event)
{
    m_widget.events.remove(event);
}

void WidgetItem::addChildWidget(WidgetItem *child)
{
    if (!m_children.contains(child)) {
        m_children.append(child);
        child->setParentItem(this);
    }
}

void WidgetItem::removeChildWidget(WidgetItem *child)
{
    m_children.removeOne(child);
    child->setParentItem(nullptr);
}

QPointF WidgetItem::snapToGrid(const QPointF &pos, qreal gridSize)
{
    return QPointF(
        qRound(pos.x() / gridSize) * gridSize,
        qRound(pos.y() / gridSize) * gridSize
    );
}

QRectF WidgetItem::boundingRect() const
{
    qreal hs = HandleSize;
    return QRectF(-hs, -hs,
                  m_widget.geometry.width() + 2 * hs,
                  m_widget.geometry.height() + 2 * hs);
}

void WidgetItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF rect(0, 0, m_widget.geometry.width(), m_widget.geometry.height());
    painter->setRenderHint(QPainter::Antialiasing, true);

    QString type = m_widget.type;

    if (type == "Button")           paintButton(painter, rect);
    else if (type == "Label")       paintLabel(painter, rect);
    else if (type == "TextField")   paintTextField(painter, rect);
    else if (type == "TextArea")    paintTextField(painter, rect);
    else if (type == "Checkbox")    paintCheckbox(painter, rect);
    else if (type == "RadioButton") paintRadioButton(painter, rect);
    else if (type == "Slider")      paintSlider(painter, rect);
    else if (type == "ProgressBar") paintProgressBar(painter, rect);
    else if (type == "Panel" || type == "ScrollPanel" ||
             type == "TabPanel" || type == "GroupBox")
                                    paintPanel(painter, rect);
    else if (type == "Image")       paintImage(painter, rect);
    else if (type == "ComboBox")    paintComboBox(painter, rect);
    else                            paintGeneric(painter, rect);

    // Рамка выделения + resize-хендлы
    if (isSelected()) {
        painter->setPen(QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
        paintResizeHandles(painter);
    }
}

// --- Рисование типов виджетов ---

void WidgetItem::paintButton(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(100, 100, 100)));
    painter->setBrush(QColor(61, 61, 61));
    painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 4, 4);

    painter->setPen(QColor(220, 220, 220));
    QString text = m_widget.properties.value("text", m_widget.name).toString();
    painter->drawText(rect, Qt::AlignCenter, text);
}

void WidgetItem::paintLabel(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QColor(200, 200, 200));
    painter->setBrush(Qt::NoBrush);

    QString text = m_widget.properties.value("text", m_widget.name).toString();
    int align = Qt::AlignLeft | Qt::AlignVCenter;
    QString alignStr = m_widget.properties.value("alignment", "left").toString();
    if (alignStr == "center") align = Qt::AlignCenter;
    else if (alignStr == "right") align = Qt::AlignRight | Qt::AlignVCenter;

    painter->drawText(rect.adjusted(4, 0, -4, 0), align, text);
}

void WidgetItem::paintTextField(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(100, 100, 100)));
    painter->setBrush(QColor(50, 50, 50));
    painter->drawRect(rect.adjusted(1, 1, -1, -1));

    QString text = m_widget.properties.value("text", "").toString();
    if (text.isEmpty())
        text = m_widget.properties.value("placeholder", "").toString();

    painter->setPen(text.isEmpty() ? QColor(120, 120, 120) : QColor(200, 200, 200));
    painter->drawText(rect.adjusted(6, 0, -6, 0), Qt::AlignLeft | Qt::AlignVCenter, text);

    // Курсор
    painter->setPen(QColor(180, 180, 180));
    qreal cursorX = rect.left() + 6 + painter->fontMetrics().horizontalAdvance(text);
    if (cursorX < rect.right() - 4) {
        painter->drawLine(QPointF(cursorX, rect.top() + 4),
                          QPointF(cursorX, rect.bottom() - 4));
    }
}

void WidgetItem::paintCheckbox(QPainter *painter, const QRectF &rect)
{
    qreal boxSize = 16.0;
    qreal y = rect.center().y() - boxSize / 2;

    // Квадратик
    QRectF box(rect.left() + 4, y, boxSize, boxSize);
    painter->setPen(QPen(QColor(120, 120, 120)));
    painter->setBrush(QColor(50, 50, 50));
    painter->drawRect(box);

    // Галочка
    bool checked = m_widget.properties.value("checked", false).toBool();
    if (checked) {
        painter->setPen(QPen(QColor(0, 180, 80), 2));
        painter->drawLine(box.left() + 3, box.center().y(),
                          box.center().x(), box.bottom() - 3);
        painter->drawLine(box.center().x(), box.bottom() - 3,
                          box.right() - 3, box.top() + 3);
    }

    // Текст
    painter->setPen(QColor(200, 200, 200));
    QString text = m_widget.properties.value("text", m_widget.name).toString();
    QRectF textRect(box.right() + 6, rect.top(), rect.width() - boxSize - 14, rect.height());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void WidgetItem::paintRadioButton(QPainter *painter, const QRectF &rect)
{
    qreal circleSize = 16.0;
    qreal y = rect.center().y() - circleSize / 2;

    // Кружок
    QRectF circle(rect.left() + 4, y, circleSize, circleSize);
    painter->setPen(QPen(QColor(120, 120, 120)));
    painter->setBrush(QColor(50, 50, 50));
    painter->drawEllipse(circle);

    // Точка
    bool selected = m_widget.properties.value("selected", false).toBool();
    if (selected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 150, 220));
        painter->drawEllipse(circle.adjusted(4, 4, -4, -4));
    }

    // Текст
    painter->setPen(QColor(200, 200, 200));
    QString text = m_widget.properties.value("text", m_widget.name).toString();
    QRectF textRect(circle.right() + 6, rect.top(),
                    rect.width() - circleSize - 14, rect.height());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void WidgetItem::paintSlider(QPainter *painter, const QRectF &rect)
{
    qreal trackY = rect.center().y();
    qreal trackLeft = rect.left() + 8;
    qreal trackRight = rect.right() - 8;

    // Полоса
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(80, 80, 80));
    painter->drawRoundedRect(QRectF(trackLeft, trackY - 2, trackRight - trackLeft, 4), 2, 2);

    // Позиция кружка
    double minVal = m_widget.properties.value("min", 0).toDouble();
    double maxVal = m_widget.properties.value("max", 100).toDouble();
    double value = m_widget.properties.value("value", 50).toDouble();
    double ratio = (maxVal > minVal) ? (value - minVal) / (maxVal - minVal) : 0.5;
    qreal knobX = trackLeft + ratio * (trackRight - trackLeft);

    // Заполненная часть
    painter->setBrush(QColor(0, 120, 215));
    painter->drawRoundedRect(QRectF(trackLeft, trackY - 2, knobX - trackLeft, 4), 2, 2);

    // Кружок
    painter->setBrush(QColor(200, 200, 200));
    painter->drawEllipse(QPointF(knobX, trackY), 7, 7);
}

void WidgetItem::paintProgressBar(QPainter *painter, const QRectF &rect)
{
    QRectF bar = rect.adjusted(2, rect.height() * 0.25, -2, -rect.height() * 0.25);

    // Фон
    painter->setPen(QPen(QColor(80, 80, 80)));
    painter->setBrush(QColor(50, 50, 50));
    painter->drawRoundedRect(bar, 3, 3);

    // Заполненная часть
    double minVal = m_widget.properties.value("min", 0).toDouble();
    double maxVal = m_widget.properties.value("max", 100).toDouble();
    double value = m_widget.properties.value("value", 40).toDouble();
    double ratio = (maxVal > minVal) ? (value - minVal) / (maxVal - minVal) : 0.0;

    if (ratio > 0) {
        QRectF filled(bar.left() + 1, bar.top() + 1,
                      (bar.width() - 2) * ratio, bar.height() - 2);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 120, 215));
        painter->drawRoundedRect(filled, 2, 2);
    }

    // Текст процента
    bool showText = m_widget.properties.value("show_text", true).toBool();
    if (showText) {
        painter->setPen(QColor(220, 220, 220));
        painter->drawText(bar, Qt::AlignCenter,
                          QString::number(int(ratio * 100)) + "%");
    }
}

void WidgetItem::paintPanel(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(80, 80, 80)));
    painter->setBrush(QColor(45, 45, 45));
    painter->drawRect(rect.adjusted(1, 1, -1, -1));

    // Заголовок (если GroupBox)
    if (m_widget.type == "GroupBox") {
        painter->setPen(QColor(180, 180, 180));
        QString title = m_widget.properties.value("text", m_widget.name).toString();
        painter->drawText(QRectF(rect.left() + 8, rect.top(), rect.width(), 20),
                          Qt::AlignLeft | Qt::AlignVCenter, title);
        painter->setPen(QPen(QColor(80, 80, 80)));
        painter->drawLine(rect.left() + 1, rect.top() + 20,
                          rect.right() - 1, rect.top() + 20);
    }
}

void WidgetItem::paintImage(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(100, 100, 100), 1, Qt::DashLine));
    painter->setBrush(QColor(40, 40, 40));
    painter->drawRect(rect.adjusted(1, 1, -1, -1));

    // Иконка-заглушка (крестик + горы)
    painter->setPen(QPen(QColor(100, 100, 100)));
    qreal cx = rect.center().x();
    qreal cy = rect.center().y();
    qreal sz = qMin(rect.width(), rect.height()) * 0.3;

    // Горы
    painter->drawLine(QPointF(cx - sz, cy + sz * 0.5),
                      QPointF(cx - sz * 0.3, cy - sz * 0.3));
    painter->drawLine(QPointF(cx - sz * 0.3, cy - sz * 0.3),
                      QPointF(cx + sz * 0.3, cy + sz * 0.5));
    painter->drawLine(QPointF(cx, cy + sz * 0.1),
                      QPointF(cx + sz, cy + sz * 0.5));

    // Солнце
    painter->drawEllipse(QPointF(cx + sz * 0.5, cy - sz * 0.3), sz * 0.2, sz * 0.2);
}

void WidgetItem::paintComboBox(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(100, 100, 100)));
    painter->setBrush(QColor(50, 50, 50));
    painter->drawRect(rect.adjusted(1, 1, -1, -1));

    // Текст
    painter->setPen(QColor(200, 200, 200));
    QString text = m_widget.properties.value("text", m_widget.name).toString();
    painter->drawText(rect.adjusted(6, 0, -24, 0), Qt::AlignLeft | Qt::AlignVCenter, text);

    // Стрелка вниз
    qreal arrowX = rect.right() - 16;
    qreal arrowY = rect.center().y();
    QPainterPath arrow;
    arrow.moveTo(arrowX - 4, arrowY - 2);
    arrow.lineTo(arrowX + 4, arrowY - 2);
    arrow.lineTo(arrowX, arrowY + 3);
    arrow.closeSubpath();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(180, 180, 180));
    painter->drawPath(arrow);
}

void WidgetItem::paintGeneric(QPainter *painter, const QRectF &rect)
{
    painter->setPen(QPen(QColor(100, 100, 100), 1, Qt::DashDotLine));
    painter->setBrush(QColor(40, 40, 40));
    painter->drawRect(rect.adjusted(1, 1, -1, -1));

    painter->setPen(QColor(150, 150, 150));
    painter->drawText(rect, Qt::AlignCenter, m_widget.type + "\n" + m_widget.name);
}

void WidgetItem::paintResizeHandles(QPainter *painter)
{
    painter->setPen(QPen(QColor(0, 120, 215)));
    painter->setBrush(QColor(0, 120, 215));

    qreal w = m_widget.geometry.width();
    qreal h = m_widget.geometry.height();
    qreal hs = HandleSize / 2;

    // 8 хендлов: углы + стороны
    QPointF handles[8] = {
        {0, 0},         // 0: top-left
        {w / 2, 0},     // 1: top-center
        {w, 0},         // 2: top-right
        {w, h / 2},     // 3: right-center
        {w, h},         // 4: bottom-right
        {w / 2, h},     // 5: bottom-center
        {0, h},         // 6: bottom-left
        {0, h / 2},     // 7: left-center
    };

    for (auto &p : handles)
        painter->drawRect(QRectF(p.x() - hs, p.y() - hs, HandleSize, HandleSize));
}

int WidgetItem::resizeHandleAt(const QPointF &localPos) const
{
    qreal w = m_widget.geometry.width();
    qreal h = m_widget.geometry.height();
    qreal hs = HandleSize;

    QPointF handles[8] = {
        {0, 0}, {w / 2, 0}, {w, 0}, {w, h / 2},
        {w, h}, {w / 2, h}, {0, h}, {0, h / 2},
    };

    for (int i = 0; i < 8; ++i) {
        QRectF r(handles[i].x() - hs, handles[i].y() - hs, hs * 2, hs * 2);
        if (r.contains(localPos))
            return i;
    }
    return -1;
}

// --- Взаимодействие ---

QVariant WidgetItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        // Уже перемещён — сигнал отправляется в mouseReleaseEvent
    }
    return QGraphicsObject::itemChange(change, value);
}

void WidgetItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = pos();
        m_resizeStartRect = QRectF(pos(), QSizeF(m_widget.geometry.width(),
                                                   m_widget.geometry.height()));

        if (isSelected()) {
            m_activeHandle = resizeHandleAt(event->pos());
        } else {
            m_activeHandle = -1;
        }

        m_dragging = true;
    }
    QGraphicsObject::mousePressEvent(event);
}

void WidgetItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_dragging && m_activeHandle >= 0) {
        // Resize
        QPointF delta = event->scenePos() - event->lastScenePos();
        qreal w = m_widget.geometry.width();
        qreal h = m_widget.geometry.height();
        QPointF p = pos();

        switch (m_activeHandle) {
        case 0: // top-left
            p += delta;
            w -= delta.x(); h -= delta.y();
            break;
        case 1: // top-center
            p.setY(p.y() + delta.y());
            h -= delta.y();
            break;
        case 2: // top-right
            p.setY(p.y() + delta.y());
            w += delta.x(); h -= delta.y();
            break;
        case 3: // right-center
            w += delta.x();
            break;
        case 4: // bottom-right
            w += delta.x(); h += delta.y();
            break;
        case 5: // bottom-center
            h += delta.y();
            break;
        case 6: // bottom-left
            p.setX(p.x() + delta.x());
            w -= delta.x(); h += delta.y();
            break;
        case 7: // left-center
            p.setX(p.x() + delta.x());
            w -= delta.x();
            break;
        }

        w = qMax(w, MinWidth);
        h = qMax(h, MinHeight);

        prepareGeometryChange();
        setPos(snapToGrid(p));
        m_widget.geometry.setWidth(w);
        m_widget.geometry.setHeight(h);
        update();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void WidgetItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;

        if (m_activeHandle >= 0) {
            // Resize закончен
            QRectF newRect(pos(), QSizeF(m_widget.geometry.width(),
                                          m_widget.geometry.height()));
            if (m_resizeStartRect != newRect)
                emit sizeChanged(m_widget.id, m_resizeStartRect, newRect);
            m_activeHandle = -1;
        } else {
            // Перемещение закончено
            QPointF snapped = snapToGrid(pos());
            setPos(snapped);
            if (m_dragStartPos != snapped)
                emit positionChanged(m_widget.id, m_dragStartPos, snapped);
        }
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

} // namespace DeltaQ

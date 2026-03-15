// Графическая сцена дизайнера UI — реализация
#include "DesignScene.h"
#include "WidgetItem.h"

#include <deltaq/UIContract.h>
#include <deltaq/UILayout.h>

#include <QPainter>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QUuid>
#include <QFont>

namespace DeltaQ {

namespace {

QPointF windowResizeHandleCenter(const QRectF &rect, int index)
{
    switch (index) {
    case 0: return QPointF(rect.right(), rect.center().y());
    case 1: return QPointF(rect.center().x(), rect.bottom());
    case 2: return rect.bottomRight();
    default: return {};
    }
}

QRectF windowFrameRectForClientRect(const QRectF &clientRect)
{
    return QRectF(clientRect.x(),
                  clientRect.y() - DesignScene::TitleBarHeight,
                  clientRect.width(),
                  clientRect.height() + DesignScene::TitleBarHeight);
}

} // namespace

DesignScene::DesignScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setBackgroundBrush(QColor(30, 30, 30));
    setSceneRect(-2000, -2000, 4000, 4000);

    // Обработка изменения выделения
    connect(this, &QGraphicsScene::selectionChanged, this, [this]() {
        auto selected = selectedItems();
        for (auto *graphicsItem : selected) {
            auto *widget = qobject_cast<WidgetItem *>(
                dynamic_cast<QGraphicsObject *>(graphicsItem));
            if (widget) {
                emit widgetSelected(widget->widgetId());
                break;
            }
        }
    });
}

QRectF DesignScene::windowFrameRect() const
{
    return windowFrameRectForClientRect(m_windowRect);
}

void DesignScene::setWindowRect(const QRectF &rect)
{
    QRectF normalized = rect;
    normalized.setWidth(qMax(rect.width(), m_windowMinimumSize.width()));
    normalized.setHeight(qMax(rect.height(), m_windowMinimumSize.height()));
    m_windowRect = normalized;
    update();
}

void DesignScene::setWindowTitle(const QString &title)
{
    const QString effectiveTitle = title.trimmed().isEmpty()
        ? QStringLiteral("Window")
        : title.trimmed();
    m_windowTitle = effectiveTitle;
    m_windowProperties["title"] = effectiveTitle;
    update();
}

void DesignScene::setWindowMinimumSize(const QSizeF &size)
{
    const QSizeF normalized(qMax(size.width(), static_cast<qreal>(DQ_UIWindowDefaultMinWidth)),
                            qMax(size.height(), static_cast<qreal>(DQ_UIWindowDefaultMinHeight)));
    m_windowMinimumSize = normalized;
    m_windowProperties["min_width"] = static_cast<int>(normalized.width());
    m_windowProperties["min_height"] = static_cast<int>(normalized.height());
    setWindowRect(m_windowRect);
}

void DesignScene::setWindowResizable(bool resizable)
{
    m_windowResizable = resizable;
    m_windowProperties["resizable"] = resizable;
    if (!m_windowResizable) {
        m_windowResizing = false;
        m_windowActiveHandle = -1;
    }
    update();
}

// --- Контейнеры ---

bool DesignScene::isContainerType(const QString &type)
{
    return isContainerUIContractType(type);
}

WidgetItem *DesignScene::containerAtPos(const QPointF &scenePos, WidgetItem *exclude) const
{
    // Ищем самый глубоко вложенный контейнер, содержащий точку
    WidgetItem *best = nullptr;
    int bestDepth = -1;

    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        WidgetItem *item = it.value();
        if (item == exclude) continue;
        if (!isContainerType(item->widgetType())) continue;

        // Проверяем, содержит ли сцено-прямоугольник виджета данную точку
        QRectF itemRect(item->scenePos(), QSizeF(item->widgetWidth(), item->widgetHeight()));
        if (!itemRect.contains(scenePos)) continue;

        // Глубина вложенности
        int depth = 0;
        QGraphicsItem *p = item->parentItem();
        while (p) { ++depth; p = p->parentItem(); }

        if (depth > bestDepth) {
            bestDepth = depth;
            best = item;
        }
    }
    return best;
}

// --- Виджеты ---

WidgetItem *DesignScene::addWidgetItem(const UIWidget &widget)
{
    if (m_widgets.contains(widget.id))
        return m_widgets[widget.id];

    auto *item = new WidgetItem(widget);
    addItem(item);
    m_widgets[widget.id] = item;

    // Подключаем сигналы
    connect(item, &WidgetItem::positionChanged, this, &DesignScene::widgetMoved);
    connect(item, &WidgetItem::sizeChanged, this, &DesignScene::widgetResized);
    connect(item, &WidgetItem::widgetDoubleClicked, this, &DesignScene::widgetDoubleClicked);

    return item;
}

void DesignScene::removeWidgetItem(const QString &widgetId)
{
    auto it = m_widgets.find(widgetId);
    if (it == m_widgets.end()) return;

    WidgetItem *item = it.value();

    // Удаляем дочерние
    for (auto *child : item->childWidgets()) {
        m_widgets.remove(child->widgetId());
    }

    removeItem(item);
    m_widgets.erase(it);
    delete item;
}

WidgetItem *DesignScene::widgetItem(const QString &widgetId) const
{
    return m_widgets.value(widgetId, nullptr);
}

// --- Загрузка/выгрузка ---

void DesignScene::loadFromLayout(const UILayout &layout)
{
    clearScene();

    m_windowProperties = layout.window.properties;
    setWindowTitle(uiWindowTitle(layout.window));
    setWindowMinimumSize(QSizeF(uiWindowMinimumWidth(layout.window),
                                uiWindowMinimumHeight(layout.window)));
    setWindowResizable(uiWindowResizable(layout.window));

    if (!layout.window.geometry.isEmpty())
        setWindowRect(layout.window.geometry);
    else
        setWindowRect(QRectF(0, 0, 800, 600));

    // Создаём виджеты из дочерних элементов window (сам window = сцена)
    for (const auto &child : layout.window.children) {
        createWidgetItems(child);
    }
}

UILayout DesignScene::toLayout(const QString &name) const
{
    UILayout layout;
    layout.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    layout.name = name;
    layout.version = "1.0.0";
    layout.window = UIWidget::create("Window", name);
    layout.window.geometry = m_windowRect;
    layout.window.properties = m_windowProperties;

    // Собираем только корневые виджеты (без родителя)
    for (auto it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        WidgetItem *item = it.value();
        if (!item->parentItem())
            layout.window.children.append(item->toUIWidget());
    }

    return layout;
}

void DesignScene::clearScene()
{
    m_widgets.clear();
    clear(); // QGraphicsScene::clear()
    m_windowRect = QRectF(0, 0, 800, 600);
    m_windowTitle = QStringLiteral("Window");
    m_windowMinimumSize = QSizeF(DQ_UIWindowDefaultMinWidth, DQ_UIWindowDefaultMinHeight);
    m_windowResizable = DQ_UIWindowDefaultResizable;
    m_windowProperties.clear();
    m_windowProperties["title"] = m_windowTitle;
    m_windowProperties["min_width"] = DQ_UIWindowDefaultMinWidth;
    m_windowProperties["min_height"] = DQ_UIWindowDefaultMinHeight;
    m_windowProperties["resizable"] = DQ_UIWindowDefaultResizable;
    m_windowSelected = false;
    m_windowResizing = false;
    m_windowActiveHandle = -1;
}

void DesignScene::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}

// --- Фон: сетка ---

void DesignScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    if (!m_gridVisible)
        return;

    painter->setPen(QPen(QColor(50, 50, 50), 0.5));

    // Мелкая сетка (10px)
    qreal left = qFloor(rect.left() / m_gridSize) * m_gridSize;
    qreal top = qFloor(rect.top() / m_gridSize) * m_gridSize;

    QVector<QLineF> lines;
    for (qreal x = left; x <= rect.right(); x += m_gridSize)
        lines.append(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y <= rect.bottom(); y += m_gridSize)
        lines.append(QLineF(rect.left(), y, rect.right(), y));
    painter->drawLines(lines);

    // Крупная сетка (50px)
    painter->setPen(QPen(QColor(65, 65, 65), 0.5));
    qreal majorSize = m_gridSize * 5;
    left = qFloor(rect.left() / majorSize) * majorSize;
    top = qFloor(rect.top() / majorSize) * majorSize;

    QVector<QLineF> majorLines;
    for (qreal x = left; x <= rect.right(); x += majorSize)
        majorLines.append(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y <= rect.bottom(); y += majorSize)
        majorLines.append(QLineF(rect.left(), y, rect.right(), y));
    painter->drawLines(majorLines);

    // --- Рамка окна по умолчанию ---
    const QRectF frameRect = windowFrameRect();
    if (!m_windowRect.isEmpty() && rect.intersects(frameRect)) {
        painter->save();

        // Тень окна
        QRectF shadowRect = frameRect.translated(4, 4);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 80));
        painter->drawRect(shadowRect);

        // Визуальная рамка окна рисуется вокруг client area; title bar вынесен
        // выше client rect, чтобы scene и runtime использовали одни координаты.
        painter->setBrush(QColor(45, 45, 48));
        painter->setPen(QPen(QColor(80, 80, 80), 1.0));
        painter->drawRect(frameRect);

        // Title bar
        QRectF titleBar(frameRect.x(), frameRect.y(),
                        frameRect.width(), TitleBarHeight);
        painter->setBrush(QColor(60, 60, 65));
        painter->setPen(Qt::NoPen);
        painter->drawRect(titleBar);

        // Кнопки окна (close/minimize/maximize) — три кружка слева
        const qreal btnRadius = 5.0;
        const qreal btnY = titleBar.center().y();
        const qreal btnStartX = titleBar.x() + 14.0;
        const qreal btnSpacing = 18.0;

        // Close (красный)
        painter->setBrush(QColor(232, 80, 80));
        painter->drawEllipse(QPointF(btnStartX, btnY), btnRadius, btnRadius);
        // Minimize (жёлтый)
        painter->setBrush(QColor(227, 190, 60));
        painter->drawEllipse(QPointF(btnStartX + btnSpacing, btnY), btnRadius, btnRadius);
        // Maximize (зелёный)
        painter->setBrush(QColor(80, 200, 80));
        painter->drawEllipse(QPointF(btnStartX + btnSpacing * 2, btnY), btnRadius, btnRadius);

        // Заголовок окна — по центру title bar
        painter->setPen(QColor(220, 220, 220));
        QFont titleFont;
        titleFont.setPixelSize(13);
        titleFont.setBold(true);
        painter->setFont(titleFont);
        painter->drawText(titleBar, Qt::AlignCenter, m_windowTitle);

        // Разделитель под title bar
        painter->setPen(QPen(QColor(80, 80, 80), 1.0));
        painter->drawLine(QPointF(m_windowRect.left(), m_windowRect.top()),
                          QPointF(m_windowRect.right(), m_windowRect.top()));

        if (m_windowSelected)
            paintWindowResizeHandles(painter);

        painter->restore();
    }
}

// --- Клик по пустому месту — выделение окна ---

void DesignScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const int handle = windowResizeHandleAt(event->scenePos());
        if (handle >= 0) {
            m_windowSelected = true;
            m_windowResizing = true;
            m_windowActiveHandle = handle;
            m_windowResizeStartRect = m_windowRect;
            update();
            event->accept();
            return;
        }
    }

    QGraphicsScene::mousePressEvent(event);

    // Если клик не попал ни по одному виджету и внутри windowRect — выделяем окно
    if (selectedItems().isEmpty() && windowFrameRect().contains(event->scenePos())) {
        m_windowSelected = true;
        update();
        emit windowSelected();
    } else if (!m_windowResizing) {
        m_windowSelected = false;
        update();
    }
}

void DesignScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_windowResizing && m_windowActiveHandle >= 0) {
        const QPointF delta = event->scenePos() - event->lastScenePos();
        qreal width = m_windowRect.width();
        qreal height = m_windowRect.height();

        switch (m_windowActiveHandle) {
        case 0:
            width += delta.x();
            break;
        case 1:
            height += delta.y();
            break;
        case 2:
            width += delta.x();
            height += delta.y();
            break;
        default:
            break;
        }

        m_windowRect.setWidth(qMax(width, m_windowMinimumSize.width()));
        m_windowRect.setHeight(qMax(height, m_windowMinimumSize.height()));
        update();
        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void DesignScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_windowResizing) {
        m_windowResizing = false;
        m_windowActiveHandle = -1;
        if (m_windowRect != m_windowResizeStartRect)
            emit windowGeometryChanged();
        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

// --- Ghost-preview при перетаскивании ---

void DesignScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawForeground(painter, rect);

    // Подсветка контейнера-цели при drag
    if (!m_dropTargetId.isEmpty()) {
        auto *target = widgetItem(m_dropTargetId);
        if (target) {
            QRectF targetRect(target->scenePos(),
                              QSizeF(target->widgetWidth(), target->widgetHeight()));
            if (rect.intersects(targetRect)) {
                painter->save();
                painter->setPen(QPen(QColor(0, 200, 80), 2.0, Qt::DashLine));
                painter->setBrush(QColor(0, 200, 80, 30));
                painter->drawRect(targetRect);
                painter->restore();
            }
        }
    }

    if (!m_showDropPreview) return;

    QRectF previewRect(m_dropPreviewPos.x(), m_dropPreviewPos.y(),
                       m_dropPreviewSize.width(), m_dropPreviewSize.height());
    if (!rect.intersects(previewRect)) return;

    painter->save();
    painter->setOpacity(0.4);
    painter->setPen(QPen(QColor(0, 150, 255), 1.5, Qt::DashLine));
    painter->setBrush(QColor(0, 150, 255, 40));
    painter->drawRect(previewRect);

    // Имя типа внутри превью
    painter->setOpacity(0.6);
    painter->setPen(QColor(200, 200, 200));
    QFont font;
    font.setPixelSize(11);
    painter->setFont(font);
    painter->drawText(previewRect, Qt::AlignCenter, displayUIContractName(m_dropPreviewType));

    painter->restore();
}

// --- Drag & Drop ---

bool DesignScene::isInsideWindowClient(const QPointF &pos) const
{
    return m_windowRect.contains(pos);
}

int DesignScene::windowResizeHandleAt(const QPointF &scenePos) const
{
    if (!m_windowResizable)
        return -1;

    for (int i = 0; i < 3; ++i) {
        const QPointF center = windowResizeHandleCenter(windowFrameRect(), i);
        const QRectF handleRect(center.x() - WindowHandleSize,
                                center.y() - WindowHandleSize,
                                WindowHandleSize * 2,
                                WindowHandleSize * 2);
        if (handleRect.contains(scenePos))
            return i;
    }

    return -1;
}

void DesignScene::paintWindowResizeHandles(QPainter *painter)
{
    if (!m_windowResizable)
        return;

    painter->save();
    painter->setPen(QPen(QColor(0, 120, 215), 1.5, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(windowFrameRect());

    painter->setPen(QPen(QColor(0, 120, 215)));
    painter->setBrush(QColor(0, 120, 215));
    for (int i = 0; i < 3; ++i) {
        const QPointF center = windowResizeHandleCenter(windowFrameRect(), i);
        painter->drawRect(QRectF(center.x() - WindowHandleSize / 2,
                                 center.y() - WindowHandleSize / 2,
                                 WindowHandleSize,
                                 WindowHandleSize));
    }
    painter->restore();
}

void DesignScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqwidget")) {
        m_dropPreviewType = QString::fromUtf8(
            event->mimeData()->data("application/x-dqwidget"));
        m_dropPreviewSize = defaultUIWidgetSize(m_dropPreviewType);

        event->acceptProposedAction();
    } else {
        QGraphicsScene::dragEnterEvent(event);
    }
}

void DesignScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqwidget")) {
        QPointF snapped = WidgetItem::snapToGrid(event->scenePos(), m_gridSize);

        // Всегда принимаем drop, но превью показываем только внутри окна
        m_dropPreviewPos = snapped;
        m_showDropPreview = isInsideWindowClient(snapped);

        // Подсветка контейнера-цели
        auto *container = containerAtPos(snapped);
        QString newTargetId = container ? container->widgetId() : QString();
        if (newTargetId != m_dropTargetId) {
            m_dropTargetId = newTargetId;
        }

        event->acceptProposedAction();
        update();
    } else {
        QGraphicsScene::dragMoveEvent(event);
    }
}

void DesignScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    m_showDropPreview = false;
    m_dropPreviewType.clear();
    m_dropTargetId.clear();
    update();
    QGraphicsScene::dragLeaveEvent(event);
}

void DesignScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    m_showDropPreview = false;

    if (event->mimeData()->hasFormat("application/x-dqwidget")) {
        QString widgetType = QString::fromUtf8(
            event->mimeData()->data("application/x-dqwidget"));
        QPointF snapped = WidgetItem::snapToGrid(event->scenePos(), m_gridSize);

        // Если вне клиентской области — корректируем позицию внутрь
        const QSizeF defaultSize = defaultUIWidgetSize(widgetType);
        const QRectF clientRect = m_windowRect;
        if (!clientRect.contains(snapped)) {
            snapped.setX(qBound(clientRect.left(), snapped.x(), clientRect.right() - defaultSize.width()));
            snapped.setY(qBound(clientRect.top(), snapped.y(), clientRect.bottom() - defaultSize.height()));
        }

        // Определяем родительский контейнер
        auto *container = containerAtPos(snapped);
        QString parentId = container ? container->widgetId() : QString();

        emit widgetDropped(widgetType, snapped, parentId);
        event->acceptProposedAction();
    } else {
        QGraphicsScene::dropEvent(event);
    }

    m_dropPreviewType.clear();
    m_dropTargetId.clear();
    update();
}

// --- Рекурсивное создание ---

WidgetItem *DesignScene::createWidgetItems(const UIWidget &widget, WidgetItem *parent)
{
    auto *item = new WidgetItem(widget);
    addItem(item);
    m_widgets[widget.id] = item;

    if (parent) {
        parent->addChildWidget(item);
    }

    // Подключаем сигналы
    connect(item, &WidgetItem::positionChanged, this, &DesignScene::widgetMoved);
    connect(item, &WidgetItem::sizeChanged, this, &DesignScene::widgetResized);
    connect(item, &WidgetItem::widgetDoubleClicked, this, &DesignScene::widgetDoubleClicked);

    // Рекурсивно дочерние
    for (const auto &child : widget.children) {
        createWidgetItems(child, item);
    }

    return item;
}

} // namespace DeltaQ

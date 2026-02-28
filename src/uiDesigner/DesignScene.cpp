// Графическая сцена дизайнера UI — реализация
#include "DesignScene.h"
#include "WidgetItem.h"

#include <deltaq/UILayout.h>

#include <QPainter>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QUuid>
#include <QFont>

namespace DeltaQ {

DesignScene::DesignScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setBackgroundBrush(QColor(30, 30, 30));
    setSceneRect(0, 0, 2000, 2000);

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
    layout.window.geometry = QRectF(0, 0, 800, 600);

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
    if (!m_windowRect.isEmpty() && rect.intersects(m_windowRect)) {
        painter->save();

        // Тень окна
        QRectF shadowRect = m_windowRect.translated(4, 4);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 80));
        painter->drawRect(shadowRect);

        // Тело окна (тёмно-серый фон, имитация SDL-окна)
        painter->setBrush(QColor(45, 45, 48));
        painter->setPen(QPen(QColor(80, 80, 80), 1.0));
        painter->drawRect(m_windowRect);

        // Title bar
        QRectF titleBar(m_windowRect.x(), m_windowRect.y(),
                        m_windowRect.width(), TitleBarHeight);
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
        painter->drawLine(QPointF(titleBar.left(), titleBar.bottom()),
                          QPointF(titleBar.right(), titleBar.bottom()));

        painter->restore();
    }
}

// --- Drag & Drop ---

void DesignScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqwidget"))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragEnterEvent(event);
}

void DesignScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqwidget"))
        event->acceptProposedAction();
    else
        QGraphicsScene::dragMoveEvent(event);
}

void DesignScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-dqwidget")) {
        QString widgetType = QString::fromUtf8(
            event->mimeData()->data("application/x-dqwidget"));
        QPointF snapped = WidgetItem::snapToGrid(event->scenePos(), m_gridSize);
        emit widgetDropped(widgetType, snapped);
        event->acceptProposedAction();
    } else {
        QGraphicsScene::dropEvent(event);
    }
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

    // Рекурсивно дочерние
    for (const auto &child : widget.children) {
        createWidgetItems(child, item);
    }

    return item;
}

} // namespace DeltaQ

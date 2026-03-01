// Движок компоновки — реализация
#include "LayoutEngine.h"
#include "WidgetItem.h"
#include "DesignScene.h"

#include <deltaq/UILayout.h>
#include <QtMath>

namespace DeltaQ {

void LayoutEngine::applyLayout(WidgetItem *container)
{
    LayoutConstraints c;
    applyLayout(container, c);
}

void LayoutEngine::applyLayout(WidgetItem *container, const LayoutConstraints &constraints)
{
    if (!container) return;

    QString layoutType = container->layoutType();
    if (layoutType == "None" || layoutType.isEmpty())
        return; // Свободное размещение

    if (layoutType == "HBox")
        applyHBoxLayout(container, constraints);
    else if (layoutType == "VBox")
        applyVBoxLayout(container, constraints);
    else if (layoutType == "Grid")
        applyGridLayout(container, constraints);
    else if (layoutType == "Flow")
        applyFlowLayout(container, constraints);
}

void LayoutEngine::applyHBoxLayout(WidgetItem *container, const LayoutConstraints &c)
{
    auto children = container->childWidgets();
    if (children.isEmpty()) return;

    qreal availableWidth = container->widgetWidth() - 2 * c.margin;
    qreal totalSpacing = c.spacing * (children.size() - 1);
    qreal childWidth = (availableWidth - totalSpacing) / children.size();
    qreal childHeight = container->widgetHeight() - 2 * c.margin;

    qreal x = c.margin;
    for (auto *child : children) {
        child->setPos(x, c.margin);
        child->setWidgetSize(childWidth, childHeight);
        x += childWidth + c.spacing;
    }
}

void LayoutEngine::applyVBoxLayout(WidgetItem *container, const LayoutConstraints &c)
{
    auto children = container->childWidgets();
    if (children.isEmpty()) return;

    qreal availableHeight = container->widgetHeight() - 2 * c.margin;
    qreal totalSpacing = c.spacing * (children.size() - 1);
    qreal childHeight = (availableHeight - totalSpacing) / children.size();
    qreal childWidth = container->widgetWidth() - 2 * c.margin;

    qreal y = c.margin;
    for (auto *child : children) {
        child->setPos(c.margin, y);
        child->setWidgetSize(childWidth, childHeight);
        y += childHeight + c.spacing;
    }
}

void LayoutEngine::applyGridLayout(WidgetItem *container, const LayoutConstraints &c)
{
    auto children = container->childWidgets();
    if (children.isEmpty()) return;

    // Авто-расчёт: количество столбцов = sqrt(N), строк = ceil(N/cols)
    int count = children.size();
    int cols = qMax(1, static_cast<int>(qCeil(qSqrt(count))));
    int rows = qMax(1, static_cast<int>(qCeil(static_cast<qreal>(count) / cols)));

    qreal availableWidth = container->widgetWidth() - 2 * c.margin;
    qreal availableHeight = container->widgetHeight() - 2 * c.margin;
    qreal cellWidth = (availableWidth - c.spacing * (cols - 1)) / cols;
    qreal cellHeight = (availableHeight - c.spacing * (rows - 1)) / rows;

    for (int i = 0; i < count; ++i) {
        int col = i % cols;
        int row = i / cols;
        qreal x = c.margin + col * (cellWidth + c.spacing);
        qreal y = c.margin + row * (cellHeight + c.spacing);
        children[i]->setPos(x, y);
        children[i]->setWidgetSize(cellWidth, cellHeight);
    }
}

void LayoutEngine::applyFlowLayout(WidgetItem *container, const LayoutConstraints &c)
{
    auto children = container->childWidgets();
    if (children.isEmpty()) return;

    qreal containerWidth = container->widgetWidth() - 2 * c.margin;
    qreal x = c.margin;
    qreal y = c.margin;
    qreal rowHeight = 0;

    for (auto *child : children) {
        qreal childWidth = child->widgetWidth();
        qreal childHeight = child->widgetHeight();

        // Перенос на следующую строку
        if (x + childWidth > containerWidth + c.margin && x > c.margin) {
            x = c.margin;
            y += rowHeight + c.spacing;
            rowHeight = 0;
        }

        child->setPos(x, y);
        x += childWidth + c.spacing;
        rowHeight = qMax(rowHeight, childHeight);
    }
}

// --- Anchor-привязки ---

void LayoutEngine::applyAnchors(WidgetItem *widget, const QSizeF &parentSize)
{
    if (!widget) return;
    UIAnchors a = widget->anchors();
    if (!a.hasAnchors()) return;

    qreal x = widget->pos().x();
    qreal y = widget->pos().y();
    qreal w = widget->widgetWidth();
    qreal h = widget->widgetHeight();
    qreal parentW = parentSize.width();
    qreal parentH = parentSize.height();

    // Горизонтальные привязки
    if (a.left && a.right) {
        // Растяжение по ширине
        x = a.leftMargin;
        w = parentW - a.leftMargin - a.rightMargin;
    } else if (a.left) {
        x = a.leftMargin;
    } else if (a.right) {
        x = parentW - a.rightMargin - w;
    } else if (a.hCenter) {
        x = (parentW - w) / 2;
    }

    // Вертикальные привязки
    if (a.top && a.bottom) {
        y = a.topMargin;
        h = parentH - a.topMargin - a.bottomMargin;
    } else if (a.top) {
        y = a.topMargin;
    } else if (a.bottom) {
        y = parentH - a.bottomMargin - h;
    } else if (a.vCenter) {
        y = (parentH - h) / 2;
    }

    widget->setPos(x, y);
    widget->setWidgetSize(w, h);
}

void LayoutEngine::applyAnchorsToChildren(WidgetItem *container)
{
    if (!container) return;
    QSizeF parentSize(container->widgetWidth(), container->widgetHeight());
    for (auto *child : container->childWidgets()) {
        applyAnchors(child, parentSize);
    }
}

void LayoutEngine::applyAnchorsToRootWidgets(DesignScene *scene)
{
    if (!scene) return;
    QRectF windowRect = scene->windowRect();
    // Клиентская область (без title bar)
    QSizeF clientSize(windowRect.width(), windowRect.height() - 30.0);

    for (auto it = scene->widgetItems().begin(); it != scene->widgetItems().end(); ++it) {
        WidgetItem *item = it.value();
        // Только корневые виджеты (без родителя)
        if (!item->parentItem()) {
            applyAnchors(item, clientSize);
        }
        // Рекурсивно для дочерних контейнеров
        if (!item->childWidgets().isEmpty()) {
            applyAnchorsToChildren(item);
        }
    }
}

} // namespace DeltaQ

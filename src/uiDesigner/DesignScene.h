// Графическая сцена дизайнера UI — сетка, drag&drop, управление виджетами
#pragma once

#include <QGraphicsScene>
#include <QMap>
#include <QString>

namespace DeltaQ {

struct UILayout;
struct UIWidget;
class WidgetItem;

class DesignScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit DesignScene(QObject *parent = nullptr);

    // Работа с виджетами
    WidgetItem *addWidgetItem(const UIWidget &widget);
    void removeWidgetItem(const QString &widgetId);
    WidgetItem *widgetItem(const QString &widgetId) const;
    QMap<QString, WidgetItem *> widgetItems() const { return m_widgets; }

    // Загрузка/выгрузка макета
    void loadFromLayout(const UILayout &layout);
    UILayout toLayout(const QString &name) const;
    void clearScene();

    // Сетка
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    void setGridSize(qreal size) { m_gridSize = size; update(); }
    qreal gridSize() const { return m_gridSize; }

signals:
    void widgetSelected(const QString &widgetId);
    void widgetDropped(const QString &widgetType, const QPointF &scenePos);
    void widgetMoved(const QString &widgetId, const QPointF &oldPos, const QPointF &newPos);
    void widgetResized(const QString &widgetId, const QRectF &oldRect, const QRectF &newRect);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    // Рекурсивное создание WidgetItem из UIWidget
    WidgetItem *createWidgetItems(const UIWidget &widget, WidgetItem *parent = nullptr);

    QMap<QString, WidgetItem *> m_widgets;
    bool m_gridVisible = true;
    qreal m_gridSize = 10.0;
};

} // namespace DeltaQ

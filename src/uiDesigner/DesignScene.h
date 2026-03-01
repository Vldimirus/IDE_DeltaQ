// Графическая сцена дизайнера UI — сетка, drag&drop, управление виджетами
#pragma once

#include <QGraphicsScene>
#include <QMap>
#include <QString>
#include <QSizeF>

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

    // Рамка окна по умолчанию
    QRectF windowRect() const { return m_windowRect; }
    void setWindowRect(const QRectF &rect) { m_windowRect = rect; update(); }
    QString windowTitle() const { return m_windowTitle; }
    void setWindowTitle(const QString &title) { m_windowTitle = title; update(); }

    // Сетка
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    void setGridSize(qreal size) { m_gridSize = size; update(); }
    qreal gridSize() const { return m_gridSize; }

    // Поиск контейнера под позицией (для drop / reparenting)
    WidgetItem *containerAtPos(const QPointF &scenePos, WidgetItem *exclude = nullptr) const;

    // Проверка, является ли тип контейнерным
    static bool isContainerType(const QString &type);

signals:
    void widgetSelected(const QString &widgetId);
    void windowSelected();  // клик по пустому месту внутри окна
    void widgetDropped(const QString &widgetType, const QPointF &scenePos, const QString &parentId);
    void widgetMoved(const QString &widgetId, const QPointF &oldPos, const QPointF &newPos);
    void widgetResized(const QString &widgetId, const QRectF &oldRect, const QRectF &newRect);
    void widgetDoubleClicked(const QString &widgetId);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    // Проверка, что позиция внутри клиентской области окна
    bool isInsideWindowClient(const QPointF &pos) const;

    // Рекурсивное создание WidgetItem из UIWidget
    WidgetItem *createWidgetItems(const UIWidget &widget, WidgetItem *parent = nullptr);

    QMap<QString, WidgetItem *> m_widgets;
    bool m_gridVisible = true;
    qreal m_gridSize = 10.0;

    // Рамка окна
    QRectF m_windowRect = QRectF(0, 0, 800, 600);
    QString m_windowTitle = QStringLiteral("Window");
    static constexpr qreal TitleBarHeight = 30.0;

    // Ghost-preview при перетаскивании
    bool m_showDropPreview = false;
    QPointF m_dropPreviewPos;
    QSizeF m_dropPreviewSize = QSizeF(120, 40);
    QString m_dropPreviewType;

    // Подсветка контейнера при drop
    QString m_dropTargetId;
};

} // namespace DeltaQ

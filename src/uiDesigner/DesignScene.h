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
    const QMap<QString, WidgetItem *> &widgetItems() const { return m_widgets; }

    // Загрузка/выгрузка макета
    void loadFromLayout(const UILayout &layout);
    UILayout toLayout(const QString &name) const;
    void clearScene();

    // Рамка окна по умолчанию
    QRectF windowRect() const { return m_windowRect; }
    void setWindowRect(const QRectF &rect) { m_windowRect = rect; update(); }
    QString windowTitle() const { return m_windowTitle; }
    void setWindowTitle(const QString &title) { m_windowTitle = title; update(); }
    void selectWindow() { m_windowSelected = true; update(); }
    void clearWindowSelection() { m_windowSelected = false; update(); }
    bool isWindowSelected() const { return m_windowSelected; }

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
    void windowGeometryChanged();
    void widgetDropped(const QString &widgetType, const QPointF &scenePos, const QString &parentId);
    void widgetMoved(const QString &widgetId, const QPointF &oldPos, const QPointF &newPos);
    void widgetResized(const QString &widgetId, const QRectF &oldRect, const QRectF &newRect);
    void widgetDoubleClicked(const QString &widgetId);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    int windowResizeHandleAt(const QPointF &scenePos) const;
    void paintWindowResizeHandles(QPainter *painter);

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
    bool m_windowSelected = false;
    bool m_windowResizing = false;
    int m_windowActiveHandle = -1;
    QRectF m_windowResizeStartRect;
    static constexpr qreal WindowHandleSize = 8.0;
    static constexpr qreal MinWindowWidth = 240.0;
    static constexpr qreal MinWindowHeight = 180.0;

    // Ghost-preview при перетаскивании
    bool m_showDropPreview = false;
    QPointF m_dropPreviewPos;
    QSizeF m_dropPreviewSize = QSizeF(120, 40);
    QString m_dropPreviewType;

    // Подсветка контейнера при drop
    QString m_dropTargetId;
};

} // namespace DeltaQ

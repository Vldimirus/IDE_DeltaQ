// Визуальный элемент UI-виджета на холсте дизайнера
#pragma once

#include <QGraphicsObject>
#include <QMap>
#include <QString>
#include <QColor>
#include <deltaq/UILayout.h>

namespace DeltaQ {

class WidgetItem : public QGraphicsObject {
    Q_OBJECT

public:
    WidgetItem(const UIWidget &widget, QGraphicsItem *parent = nullptr);

    QString widgetId() const { return m_widget.id; }
    QString widgetType() const { return m_widget.type; }
    QString widgetName() const { return m_widget.name; }

    // Доступ к данным
    UIWidget toUIWidget() const;
    void fromUIWidget(const UIWidget &widget);

    // Свойства
    QVariant property(const QString &key) const;
    void setWidgetProperty(const QString &key, const QVariant &value);
    QMap<QString, QVariant> allProperties() const { return m_widget.properties; }

    // Геометрия
    void setWidgetSize(qreal w, qreal h);
    qreal widgetWidth() const { return m_widget.geometry.width(); }
    qreal widgetHeight() const { return m_widget.geometry.height(); }

    // Layout
    QString layoutType() const { return m_widget.layout; }
    void setLayoutType(const QString &layout) { m_widget.layout = layout; }

    // События
    QMap<QString, QString> events() const { return m_widget.events; }
    void setEvent(const QString &event, const QString &handler);
    void removeEvent(const QString &event);

    // Дочерние виджеты
    void addChildWidget(WidgetItem *child);
    void removeChildWidget(WidgetItem *child);
    QVector<WidgetItem *> childWidgets() const { return m_children; }

    // Snap к сетке
    static QPointF snapToGrid(const QPointF &pos, qreal gridSize = 10.0);

    // QGraphicsItem интерфейс
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // Индекс resize-хендла под курсором (-1 если нет)
    int resizeHandleAt(const QPointF &localPos) const;

signals:
    void positionChanged(const QString &id, const QPointF &oldPos, const QPointF &newPos);
    void sizeChanged(const QString &id, const QRectF &oldRect, const QRectF &newRect);
    void propertyChanged(const QString &id, const QString &key, const QVariant &value);
    void widgetDoubleClicked(const QString &widgetId);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    // Рисование по типу
    void paintButton(QPainter *painter, const QRectF &rect);
    void paintLabel(QPainter *painter, const QRectF &rect);
    void paintTextField(QPainter *painter, const QRectF &rect);
    void paintCheckbox(QPainter *painter, const QRectF &rect);
    void paintRadioButton(QPainter *painter, const QRectF &rect);
    void paintSlider(QPainter *painter, const QRectF &rect);
    void paintProgressBar(QPainter *painter, const QRectF &rect);
    void paintPanel(QPainter *painter, const QRectF &rect);
    void paintImage(QPainter *painter, const QRectF &rect);
    void paintComboBox(QPainter *painter, const QRectF &rect);
    void paintGeneric(QPainter *painter, const QRectF &rect);
    void paintResizeHandles(QPainter *painter);

    UIWidget m_widget;
    QVector<WidgetItem *> m_children;

    // Перетаскивание / resize
    QPointF m_dragStartPos;
    QRectF m_resizeStartRect;
    int m_activeHandle = -1; // -1 = перемещение, 0-7 = resize-хендлы
    bool m_dragging = false;

    static constexpr qreal HandleSize = 6.0;
    static constexpr qreal MinWidth = 20.0;
    static constexpr qreal MinHeight = 20.0;
};

} // namespace DeltaQ

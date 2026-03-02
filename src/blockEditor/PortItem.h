// Визуальный порт узла — кружок (data) или треугольник (exec) с подписью
#pragma once

#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QString>
#include <QVector>
#include <deltaq/Module.h> // PortDirection, PortKind

namespace DeltaQ {

class NodeItem;
class ConnectionItem;

class PortItem : public QGraphicsItem {
public:
    PortItem(const QString &name, const QString &type,
             PortDirection direction, PortKind kind, NodeItem *parent);

    QString portName() const { return m_name; }
    QString portType() const { return m_type; }
    PortDirection direction() const { return m_direction; }
    PortKind portKind() const { return m_kind; }
    NodeItem *parentNode() const { return m_parentNode; }

    // Соединения, привязанные к этому порту
    void addConnection(ConnectionItem *conn);
    void removeConnection(ConnectionItem *conn);
    QVector<ConnectionItem *> connections() const { return m_connections; }

    // Центр порта в координатах сцены
    QPointF centerInScene() const;

    // Цвет по типу данных
    static QColor colorForType(const QString &type);

    // QGraphicsItem интерфейс
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    QPainterPath shape() const override;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void updateAppearance();

    QString m_name;
    QString m_type;
    PortDirection m_direction;
    PortKind m_kind;
    NodeItem *m_parentNode;
    QGraphicsTextItem *m_label = nullptr;
    QVector<ConnectionItem *> m_connections;
    bool m_hovered = false;

    static constexpr qreal NormalRadius = 8.0;
    static constexpr qreal HoverRadius = 11.0;
};

} // namespace DeltaQ

// Визуальный порт узла — кружок с подписью (input слева, output справа)
#pragma once

#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QString>
#include <QVector>
#include <deltaq/Module.h> // PortDirection

namespace DeltaQ {

class NodeItem;
class ConnectionItem;

class PortItem : public QGraphicsEllipseItem {
public:
    PortItem(const QString &name, const QString &type,
             PortDirection direction, NodeItem *parent);

    QString portName() const { return m_name; }
    QString portType() const { return m_type; }
    PortDirection direction() const { return m_direction; }
    NodeItem *parentNode() const { return m_parentNode; }

    // Соединения, привязанные к этому порту
    void addConnection(ConnectionItem *conn);
    void removeConnection(ConnectionItem *conn);
    QVector<ConnectionItem *> connections() const { return m_connections; }

    // Центр порта в координатах сцены
    QPointF centerInScene() const;

    // Цвет по типу данных
    static QColor colorForType(const QString &type);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    void updateAppearance();

    QString m_name;
    QString m_type;
    PortDirection m_direction;
    NodeItem *m_parentNode;
    QGraphicsTextItem *m_label = nullptr;
    QVector<ConnectionItem *> m_connections;

    static constexpr qreal NormalRadius = 8.0;
    static constexpr qreal HoverRadius = 11.0;
};

} // namespace DeltaQ

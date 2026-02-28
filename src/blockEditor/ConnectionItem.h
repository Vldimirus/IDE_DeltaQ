// Визуальное соединение между портами — кривая Безье
#pragma once

#include <QGraphicsPathItem>
#include <QString>

namespace DeltaQ {

class PortItem;

class ConnectionItem : public QGraphicsPathItem {
public:
    ConnectionItem(PortItem *sourcePort, PortItem *destPort,
                   QGraphicsItem *parent = nullptr);

    // Конструктор для временного соединения (при перетаскивании)
    explicit ConnectionItem(PortItem *sourcePort,
                            QGraphicsItem *parent = nullptr);

    PortItem *sourcePort() const { return m_sourcePort; }
    PortItem *destPort() const { return m_destPort; }

    void setDestPort(PortItem *port);
    void setTempEndPoint(const QPointF &point);

    // Пересчёт пути Безье
    void updatePath();

    // Подсветка
    void setHighlighted(bool on);

protected:
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    void computeBezierPath();

    PortItem *m_sourcePort = nullptr;
    PortItem *m_destPort = nullptr;
    QPointF m_tempEndPoint;
    bool m_highlighted = false;
    QString m_dataType;
};

} // namespace DeltaQ

// Визуальный узел графа — заголовок, порты, тело
#pragma once

#include <QGraphicsObject>
#include <QVector>
#include <QString>
#include <QColor>
#include <deltaq/Module.h> // PortDirection

namespace DeltaQ {

class PortItem;
struct Module;

class NodeItem : public QGraphicsObject {
    Q_OBJECT

public:
    NodeItem(const QString &nodeId, const QString &moduleName,
             const QString &category, QGraphicsItem *parent = nullptr);

    QString nodeId() const { return m_nodeId; }
    QString moduleName() const { return m_moduleName; }

    // Добавление портов
    void addInputPort(const QString &name, const QString &type,
                      PortKind kind = PortKind::Data);
    void addOutputPort(const QString &name, const QString &type,
                       PortKind kind = PortKind::Data);

    // Поиск порта по имени и направлению
    PortItem *findPort(const QString &name, PortDirection dir) const;
    QVector<PortItem *> inputPorts() const { return m_inputPorts; }
    QVector<PortItem *> outputPorts() const { return m_outputPorts; }

    // Визуальные состояния
    void setHighlighted(bool on);
    void setCompleted(bool on);
    void setError(bool on, const QString &message = {});

    bool isHighlighted() const { return m_highlighted; }
    bool isCompleted() const { return m_completed; }
    bool hasError() const { return m_hasError; }

    // QGraphicsItem интерфейс
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // Цвет категории
    static QColor categoryColor(const QString &category);

signals:
    void positionChanged(const QString &nodeId, const QPointF &newPos);
    void doubleClicked(const QString &nodeId);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    qreal computeWidth() const;
    qreal computeHeight() const;
    void updatePortPositions();

    QString m_nodeId;
    QString m_moduleName;
    QString m_category;

    QVector<PortItem *> m_inputPorts;
    QVector<PortItem *> m_outputPorts;

    bool m_highlighted = false;
    bool m_completed = false;
    bool m_hasError = false;
    QString m_errorMessage;
    qreal m_width = 180.0;
    qreal m_height = 65.0;

    // Отслеживание перемещения для MoveCommand
    QPointF m_dragStartPos;

    static constexpr qreal MinWidth = 180.0;
    static constexpr qreal HeaderHeight = 30.0;
    static constexpr qreal PortSpacing = 26.0;
    static constexpr qreal BottomPadding = 10.0;
    static constexpr qreal SidePadding = 16.0;
    static constexpr qreal PortLabelGap = 12.0;
    static constexpr qreal CenterGap = 44.0;
    static constexpr qreal ExecPortSpacing = 18.0;
    static constexpr qreal ExecOuterPadding = 18.0;
};

} // namespace DeltaQ

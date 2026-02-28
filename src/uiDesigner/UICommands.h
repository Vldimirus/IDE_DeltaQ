// Команды UI дизайнера — undo/redo для всех операций
#pragma once

#include "../core/CommandBus.h"
#include <deltaq/UILayout.h>
#include <QPointF>
#include <QRectF>
#include <QVariant>
#include <QVector>

namespace DeltaQ {

class DesignScene;

// Добавление виджета
class AddWidgetCommand : public Command {
public:
    AddWidgetCommand(DesignScene *scene, UILayout *layout, const UIWidget &widget);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    UIWidget m_widget;
};

// Удаление виджета (с сохранением для undo)
class RemoveWidgetCommand : public Command {
public:
    RemoveWidgetCommand(DesignScene *scene, UILayout *layout, const QString &widgetId);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    UIWidget m_savedWidget;
};

// Перемещение виджета
class MoveWidgetCommand : public Command {
public:
    MoveWidgetCommand(DesignScene *scene, UILayout *layout,
                      const QString &widgetId, const QPointF &oldPos, const QPointF &newPos);
    void execute() override;
    void undo() override;
    QString description() const override;
    bool mergeWith(const Command *other) override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    QPointF m_oldPos;
    QPointF m_newPos;
};

// Изменение размера виджета
class ResizeWidgetCommand : public Command {
public:
    ResizeWidgetCommand(DesignScene *scene, UILayout *layout,
                        const QString &widgetId,
                        const QRectF &oldRect, const QRectF &newRect);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    QRectF m_oldRect;
    QRectF m_newRect;
};

// Изменение свойства виджета
class ChangeWidgetPropertyCommand : public Command {
public:
    ChangeWidgetPropertyCommand(DesignScene *scene, UILayout *layout,
                                 const QString &widgetId, const QString &propertyName,
                                 const QVariant &oldValue, const QVariant &newValue);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    QString m_propertyName;
    QVariant m_oldValue;
    QVariant m_newValue;
};

// Изменение layout контейнера
class ChangeLayoutCommand : public Command {
public:
    ChangeLayoutCommand(DesignScene *scene, UILayout *layout,
                        const QString &widgetId,
                        const QString &oldLayoutType, const QString &newLayoutType);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    QString m_oldLayoutType;
    QString m_newLayoutType;
};

// Привязка/отвязка обработчика события
class BindEventCommand : public Command {
public:
    BindEventCommand(DesignScene *scene, UILayout *layout,
                     const QString &widgetId, const QString &eventName,
                     const QString &oldHandler, const QString &newHandler);
    void execute() override;
    void undo() override;
    QString description() const override;

private:
    DesignScene *m_scene;
    UILayout *m_layout;
    QString m_widgetId;
    QString m_eventName;
    QString m_oldHandler;
    QString m_newHandler;
};

} // namespace DeltaQ

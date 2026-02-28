// Команды UI дизайнера — реализация
#include "UICommands.h"
#include "DesignScene.h"
#include "WidgetItem.h"
#include "LayoutEngine.h"

namespace DeltaQ {

// --- Вспомогательные: поиск UIWidget в дереве ---

static UIWidget *findWidgetInTree(UIWidget &root, const QString &id)
{
    if (root.id == id) return &root;
    for (auto &child : root.children) {
        auto *found = findWidgetInTree(child, id);
        if (found) return found;
    }
    return nullptr;
}

static void removeWidgetFromTree(UIWidget &root, const QString &id)
{
    for (int i = 0; i < root.children.size(); ++i) {
        if (root.children[i].id == id) {
            root.children.removeAt(i);
            return;
        }
        removeWidgetFromTree(root.children[i], id);
    }
}

// --- AddWidgetCommand ---

AddWidgetCommand::AddWidgetCommand(DesignScene *scene, UILayout *layout,
                                     const UIWidget &widget)
    : m_scene(scene), m_layout(layout), m_widget(widget)
{
}

void AddWidgetCommand::execute()
{
    m_scene->addWidgetItem(m_widget);
    m_layout->window.children.append(m_widget);
}

void AddWidgetCommand::undo()
{
    m_scene->removeWidgetItem(m_widget.id);
    removeWidgetFromTree(m_layout->window, m_widget.id);
}

QString AddWidgetCommand::description() const
{
    return QObject::tr("Add widget '%1'").arg(m_widget.name);
}

// --- RemoveWidgetCommand ---

RemoveWidgetCommand::RemoveWidgetCommand(DesignScene *scene, UILayout *layout,
                                           const QString &widgetId)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
{
}

void RemoveWidgetCommand::execute()
{
    // Сохраняем виджет для undo
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        m_savedWidget = item->toUIWidget();

    m_scene->removeWidgetItem(m_widgetId);
    removeWidgetFromTree(m_layout->window, m_widgetId);
}

void RemoveWidgetCommand::undo()
{
    m_scene->addWidgetItem(m_savedWidget);
    m_layout->window.children.append(m_savedWidget);
}

QString RemoveWidgetCommand::description() const
{
    return QObject::tr("Remove widget '%1'").arg(m_savedWidget.name);
}

// --- MoveWidgetCommand ---

MoveWidgetCommand::MoveWidgetCommand(DesignScene *scene, UILayout *layout,
                                       const QString &widgetId,
                                       const QPointF &oldPos, const QPointF &newPos)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_oldPos(oldPos), m_newPos(newPos)
{
}

void MoveWidgetCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setPos(m_newPos);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        w->geometry.setX(m_newPos.x());
        w->geometry.setY(m_newPos.y());
    }
}

void MoveWidgetCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setPos(m_oldPos);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        w->geometry.setX(m_oldPos.x());
        w->geometry.setY(m_oldPos.y());
    }
}

QString MoveWidgetCommand::description() const
{
    return QObject::tr("Move widget");
}

bool MoveWidgetCommand::mergeWith(const Command *other)
{
    auto *cmd = dynamic_cast<const MoveWidgetCommand *>(other);
    if (!cmd || cmd->m_widgetId != m_widgetId)
        return false;
    m_newPos = cmd->m_newPos;
    return true;
}

// --- ResizeWidgetCommand ---

ResizeWidgetCommand::ResizeWidgetCommand(DesignScene *scene, UILayout *layout,
                                           const QString &widgetId,
                                           const QRectF &oldRect, const QRectF &newRect)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_oldRect(oldRect), m_newRect(newRect)
{
}

void ResizeWidgetCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        item->setPos(m_newRect.topLeft());
        item->setWidgetSize(m_newRect.width(), m_newRect.height());
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->geometry = m_newRect;
}

void ResizeWidgetCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        item->setPos(m_oldRect.topLeft());
        item->setWidgetSize(m_oldRect.width(), m_oldRect.height());
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->geometry = m_oldRect;
}

QString ResizeWidgetCommand::description() const
{
    return QObject::tr("Resize widget");
}

// --- ChangeWidgetPropertyCommand ---

ChangeWidgetPropertyCommand::ChangeWidgetPropertyCommand(
    DesignScene *scene, UILayout *layout,
    const QString &widgetId, const QString &propertyName,
    const QVariant &oldValue, const QVariant &newValue)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_propertyName(propertyName), m_oldValue(oldValue), m_newValue(newValue)
{
}

void ChangeWidgetPropertyCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setWidgetProperty(m_propertyName, m_newValue);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->properties[m_propertyName] = m_newValue;
}

void ChangeWidgetPropertyCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setWidgetProperty(m_propertyName, m_oldValue);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        if (m_oldValue.isValid())
            w->properties[m_propertyName] = m_oldValue;
        else
            w->properties.remove(m_propertyName);
    }
}

QString ChangeWidgetPropertyCommand::description() const
{
    return QObject::tr("Change property '%1'").arg(m_propertyName);
}

// --- ChangeLayoutCommand ---

ChangeLayoutCommand::ChangeLayoutCommand(DesignScene *scene, UILayout *layout,
                                           const QString &widgetId,
                                           const QString &oldLayoutType,
                                           const QString &newLayoutType)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_oldLayoutType(oldLayoutType), m_newLayoutType(newLayoutType)
{
}

void ChangeLayoutCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        item->setLayoutType(m_newLayoutType);
        LayoutEngine::applyLayout(item);
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->layout = m_newLayoutType;
}

void ChangeLayoutCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        item->setLayoutType(m_oldLayoutType);
        LayoutEngine::applyLayout(item);
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->layout = m_oldLayoutType;
}

QString ChangeLayoutCommand::description() const
{
    return QObject::tr("Change layout to '%1'").arg(m_newLayoutType);
}

// --- BindEventCommand ---

BindEventCommand::BindEventCommand(DesignScene *scene, UILayout *layout,
                                     const QString &widgetId, const QString &eventName,
                                     const QString &oldHandler, const QString &newHandler)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_eventName(eventName), m_oldHandler(oldHandler), m_newHandler(newHandler)
{
}

void BindEventCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        if (m_newHandler.isEmpty())
            item->removeEvent(m_eventName);
        else
            item->setEvent(m_eventName, m_newHandler);
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        if (m_newHandler.isEmpty())
            w->events.remove(m_eventName);
        else
            w->events[m_eventName] = m_newHandler;
    }
}

void BindEventCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item) {
        if (m_oldHandler.isEmpty())
            item->removeEvent(m_eventName);
        else
            item->setEvent(m_eventName, m_oldHandler);
    }

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        if (m_oldHandler.isEmpty())
            w->events.remove(m_eventName);
        else
            w->events[m_eventName] = m_oldHandler;
    }
}

QString BindEventCommand::description() const
{
    return QObject::tr("Bind event '%1'").arg(m_eventName);
}

} // namespace DeltaQ

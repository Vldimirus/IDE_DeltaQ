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

// --- Поиск родителя в дереве ---

static UIWidget *findParentOf(UIWidget &root, const QString &childId)
{
    for (auto &child : root.children) {
        if (child.id == childId) return &root;
        auto *found = findParentOf(child, childId);
        if (found) return found;
    }
    return nullptr;
}

// --- AddWidgetCommand ---

AddWidgetCommand::AddWidgetCommand(DesignScene *scene, UILayout *layout,
                                     const UIWidget &widget, const QString &parentId)
    : m_scene(scene), m_layout(layout), m_widget(widget), m_parentId(parentId)
{
}

void AddWidgetCommand::execute()
{
    auto *item = m_scene->addWidgetItem(m_widget);

    if (!m_parentId.isEmpty()) {
        // Добавить в контейнер на сцене
        auto *parentItem = m_scene->widgetItem(m_parentId);
        if (parentItem) {
            // Координаты относительно родителя
            QPointF relPos = item->pos() - parentItem->scenePos();
            parentItem->addChildWidget(item);
            item->setPos(relPos);
        }
        // Добавить в дерево данных
        auto *parentWidget = findWidgetInTree(m_layout->window, m_parentId);
        if (parentWidget)
            parentWidget->children.append(m_widget);
    } else {
        m_layout->window.children.append(m_widget);
    }
}

void AddWidgetCommand::undo()
{
    // Убрать из родителя на сцене
    if (!m_parentId.isEmpty()) {
        auto *parentItem = m_scene->widgetItem(m_parentId);
        auto *childItem = m_scene->widgetItem(m_widget.id);
        if (parentItem && childItem)
            parentItem->removeChildWidget(childItem);
    }
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
        // moveTopLeft сохраняет ширину/высоту (setX/setY ломают размер!)
        w->geometry.moveTopLeft(m_newPos);
    }
}

void MoveWidgetCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setPos(m_oldPos);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w) {
        w->geometry.moveTopLeft(m_oldPos);
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

// --- ReparentWidgetCommand ---

ReparentWidgetCommand::ReparentWidgetCommand(DesignScene *scene, UILayout *layout,
                                               const QString &widgetId,
                                               const QString &oldParentId, const QString &newParentId,
                                               const QPointF &oldPos, const QPointF &newPos)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_oldParentId(oldParentId), m_newParentId(newParentId)
    , m_oldPos(oldPos), m_newPos(newPos)
{
}

static void reparentOnScene(DesignScene *scene, const QString &widgetId,
                             const QString &fromParentId, const QString &toParentId,
                             const QPointF &newScenePos)
{
    auto *item = scene->widgetItem(widgetId);
    if (!item) return;

    // Снять с текущего родителя
    if (!fromParentId.isEmpty()) {
        auto *oldParent = scene->widgetItem(fromParentId);
        if (oldParent)
            oldParent->removeChildWidget(item);
    }

    // Добавить к новому родителю и пересчитать позицию из сценических координат
    if (!toParentId.isEmpty()) {
        auto *newParent = scene->widgetItem(toParentId);
        if (newParent) {
            newParent->addChildWidget(item);
            item->setPos(newParent->mapFromScene(newScenePos));
        } else {
            item->setPos(newScenePos);
        }
    } else {
        // Переносим на корневой уровень — позиция = сценические координаты
        item->setPos(newScenePos);
    }
}

static void reparentInLayout(UILayout *layout, const QString &widgetId,
                              const QString &fromParentId, const QString &toParentId)
{
    // Находим виджет и сохраняем копию
    auto *widget = findWidgetInTree(layout->window, widgetId);
    if (!widget) return;
    UIWidget widgetCopy = *widget;
    widgetCopy.children = widget->children; // сохраняем дочерних

    // Удаляем из старого родителя
    if (!fromParentId.isEmpty()) {
        auto *oldParent = findWidgetInTree(layout->window, fromParentId);
        if (oldParent) {
            for (int i = 0; i < oldParent->children.size(); ++i) {
                if (oldParent->children[i].id == widgetId) {
                    oldParent->children.removeAt(i);
                    break;
                }
            }
        }
    } else {
        removeWidgetFromTree(layout->window, widgetId);
    }

    // Добавляем к новому родителю
    if (!toParentId.isEmpty()) {
        auto *newParent = findWidgetInTree(layout->window, toParentId);
        if (newParent)
            newParent->children.append(widgetCopy);
    } else {
        layout->window.children.append(widgetCopy);
    }
}

void ReparentWidgetCommand::execute()
{
    reparentOnScene(m_scene, m_widgetId, m_oldParentId, m_newParentId, m_newPos);
    reparentInLayout(m_layout, m_widgetId, m_oldParentId, m_newParentId);
}

void ReparentWidgetCommand::undo()
{
    reparentOnScene(m_scene, m_widgetId, m_newParentId, m_oldParentId, m_oldPos);
    reparentInLayout(m_layout, m_widgetId, m_newParentId, m_oldParentId);
}

QString ReparentWidgetCommand::description() const
{
    return QObject::tr("Reparent widget");
}

// --- ChangeAnchorsCommand ---

ChangeAnchorsCommand::ChangeAnchorsCommand(DesignScene *scene, UILayout *layout,
                                             const QString &widgetId,
                                             const UIAnchors &oldAnchors, const UIAnchors &newAnchors)
    : m_scene(scene), m_layout(layout), m_widgetId(widgetId)
    , m_oldAnchors(oldAnchors), m_newAnchors(newAnchors)
{
}

void ChangeAnchorsCommand::execute()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setAnchors(m_newAnchors);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->anchors = m_newAnchors;
}

void ChangeAnchorsCommand::undo()
{
    auto *item = m_scene->widgetItem(m_widgetId);
    if (item)
        item->setAnchors(m_oldAnchors);

    auto *w = findWidgetInTree(m_layout->window, m_widgetId);
    if (w)
        w->anchors = m_oldAnchors;
}

QString ChangeAnchorsCommand::description() const
{
    return QObject::tr("Change anchors");
}

} // namespace DeltaQ

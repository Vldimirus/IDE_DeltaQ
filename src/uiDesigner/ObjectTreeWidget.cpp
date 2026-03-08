// Дерево объектов UI — реализация
#include "ObjectTreeWidget.h"
#include "DesignScene.h"
#include "WidgetItem.h"

#include <deltaq/UIContract.h>

#include <QHeaderView>

namespace DeltaQ {

ObjectTreeWidget::ObjectTreeWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel(tr("Objects"));
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tree);

    // Клик по элементу дерева → сигнал widgetSelected
    connect(m_tree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (!current) return;
        QString widgetId = current->data(0, Qt::UserRole).toString();
        // Пустой widgetId означает клик по "Window" (корневой элемент)
        emit widgetSelected(widgetId);
    });
}

void ObjectTreeWidget::rebuild(DesignScene *scene)
{
    m_scene = scene;
    m_tree->blockSignals(true);
    m_tree->clear();

    if (!scene) {
        m_tree->blockSignals(false);
        return;
    }

    // Корневой элемент — Window
    auto *rootItem = new QTreeWidgetItem(m_tree, {tr("Window")});
    rootItem->setExpanded(true);

    // Добавить только корневые виджеты (без родителя)
    auto widgets = scene->widgetItems();
    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        WidgetItem *item = it.value();
        if (!item->parentItem()) {
            addWidgetToTree(item, rootItem);
        }
    }

    m_tree->expandAll();
    m_tree->blockSignals(false);
}

void ObjectTreeWidget::selectWindow()
{
    m_tree->blockSignals(true);
    if (m_tree->topLevelItemCount() > 0)
        m_tree->setCurrentItem(m_tree->topLevelItem(0));
    m_tree->blockSignals(false);
}

void ObjectTreeWidget::selectWidget(const QString &widgetId)
{
    m_tree->blockSignals(true);

    // Поиск элемента по widgetId
    std::function<QTreeWidgetItem *(QTreeWidgetItem *)> findItem;
    findItem = [&](QTreeWidgetItem *parent) -> QTreeWidgetItem * {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto *child = parent->child(i);
            if (child->data(0, Qt::UserRole).toString() == widgetId)
                return child;
            auto *found = findItem(child);
            if (found) return found;
        }
        return nullptr;
    };

    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *found = findItem(m_tree->topLevelItem(i));
        if (found) {
            m_tree->setCurrentItem(found);
            break;
        }
    }

    m_tree->blockSignals(false);
}

void ObjectTreeWidget::addWidgetToTree(WidgetItem *item, QTreeWidgetItem *parentTreeItem)
{
    const QString displayType = item->widgetDisplayType();
    const QString contractType = item->widgetContractType();
    QString label = QString("%1 (%2)").arg(item->widgetName(), displayType);
    if (!contractType.isEmpty())
        label += QString(" [%1]").arg(contractType);
    auto *treeItem = new QTreeWidgetItem(parentTreeItem, {label});
    treeItem->setData(0, Qt::UserRole, item->widgetId());

    // Рекурсивно добавить дочерние
    for (auto *child : item->childWidgets()) {
        addWidgetToTree(child, treeItem);
    }
}

} // namespace DeltaQ

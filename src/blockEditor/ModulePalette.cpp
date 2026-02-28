// Палитра модулей — реализация дерева категорий, поиска и drag&drop
#include "ModulePalette.h"
#include "../core/ModuleRegistry.h"
#include <deltaq/Module.h>

#include <QHeaderView>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMouseEvent>

namespace DeltaQ {

ModulePalette::ModulePalette(ModuleRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search modules..."));
    layout->addWidget(m_searchEdit);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setDragEnabled(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tree);

    // Фильтрация при вводе
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ModulePalette::filterTree);

    // Двойной клик — добавить модуль в центр
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        startDragForItem(item);
    });

    // Drag из дерева
    m_tree->setDragDropMode(QAbstractItemView::DragOnly);

    buildTree();
}

void ModulePalette::rebuildTree()
{
    buildTree();
}

void ModulePalette::setFilter(const QString &text)
{
    m_searchEdit->setText(text);
}

void ModulePalette::buildTree()
{
    m_tree->clear();

    if (!m_registry) return;

    QStringList cats = m_registry->categories();
    cats.sort();

    for (const auto &cat : cats) {
        auto *catItem = new QTreeWidgetItem(m_tree, {cat});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);

        // Цветная иконка по категории
        QPixmap px(12, 12);
        QColor color;
        if (cat == "math")        color = QColor(50, 100, 180);
        else if (cat == "logic")  color = QColor(50, 140, 80);
        else if (cat == "io")     color = QColor(200, 120, 40);
        else if (cat == "string") color = QColor(140, 80, 180);
        else                      color = QColor(100, 100, 100);
        px.fill(color);
        catItem->setIcon(0, QIcon(px));

        auto modules = m_registry->modulesByCategory(cat);
        for (const auto *mod : modules) {
            auto *modItem = new QTreeWidgetItem(catItem, {mod->name});
            modItem->setData(0, Qt::UserRole, mod->id);
            modItem->setFlags(modItem->flags() | Qt::ItemIsDragEnabled);

            // Tooltip: описание + порты
            QString tip = mod->description;
            if (!mod->inputs.isEmpty()) {
                tip += "\n\nInputs:";
                for (const auto &p : mod->inputs)
                    tip += QString("\n  %1 (%2)").arg(p.name, p.type);
            }
            if (!mod->outputs.isEmpty()) {
                tip += "\n\nOutputs:";
                for (const auto &p : mod->outputs)
                    tip += QString("\n  %1 (%2)").arg(p.name, p.type);
            }
            modItem->setToolTip(0, tip);
        }

        catItem->setExpanded(true);
    }
}

void ModulePalette::filterTree(const QString &text)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *catItem = m_tree->topLevelItem(i);
        bool catVisible = false;

        for (int j = 0; j < catItem->childCount(); ++j) {
            auto *modItem = catItem->child(j);
            bool match = text.isEmpty() ||
                         modItem->text(0).contains(text, Qt::CaseInsensitive);
            modItem->setHidden(!match);
            if (match) catVisible = true;
        }

        catItem->setHidden(!catVisible);
    }
}

void ModulePalette::startDragForItem(QTreeWidgetItem *item)
{
    if (!item || !item->parent()) return; // Только элементы модулей, не категории

    QString moduleId = item->data(0, Qt::UserRole).toString();
    if (moduleId.isEmpty()) return;

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setData("application/x-dqmodule", moduleId.toUtf8());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

} // namespace DeltaQ

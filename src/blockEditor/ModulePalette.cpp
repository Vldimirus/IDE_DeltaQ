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

void ModulePalette::setLanguageFilter(const QString &lang)
{
    m_languageFilter = lang;
    buildTree();
}

// Цвет по категории
static QColor categoryColor(const QString &cat)
{
    if (cat == "math")        return QColor(50, 100, 180);
    if (cat == "logic")       return QColor(50, 140, 80);
    if (cat == "io")          return QColor(200, 120, 40);
    if (cat == "string")      return QColor(140, 80, 180);
    if (cat == "conversion")  return QColor(100, 140, 200);
    if (cat == "control")     return QColor(180, 100, 50);
    if (cat == "ui")          return QColor(220, 80, 80);
    return QColor(100, 100, 100);
}

// Tooltip для модуля
static QString moduleTooltip(const Module *mod)
{
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
    return tip;
}

// Добавить модуль в дерево с проверкой языка
QTreeWidgetItem *addModuleItem(QTreeWidgetItem *parent, const Module *mod,
                                const QString &languageFilter)
{
    if (!languageFilter.isEmpty()) {
        bool compatible = (mod->language == languageFilter) ||
            (languageFilter == "c" && mod->language == "cpp") ||
            (languageFilter == "cpp" && mod->language == "c") ||
            mod->language.isEmpty();
        if (!compatible) return nullptr;
    }

    auto *item = new QTreeWidgetItem(parent, {mod->name});
    item->setData(0, Qt::UserRole, mod->id);
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    item->setToolTip(0, moduleTooltip(mod));
    return item;
}

void ModulePalette::buildTree()
{
    m_tree->clear();

    if (!m_registry) return;

    auto allModules = m_registry->allModules();

    // === Секция 1: Стандартная библиотека (core) ===
    auto *coreRoot = new QTreeWidgetItem(m_tree, {tr("Стандартная библиотека")});
    coreRoot->setFlags(coreRoot->flags() & ~Qt::ItemIsDragEnabled);
    QFont boldFont = coreRoot->font(0);
    boldFont.setBold(true);
    coreRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(50, 120, 200));
        coreRoot->setIcon(0, QIcon(px));
    }

    // Собираем core-модули по подкатегориям
    QMap<QString, QVector<const Module *>> coreByCategory;
    for (const auto *mod : allModules) {
        if (mod->origin == "core")
            coreByCategory[mod->category].append(mod);
    }

    QStringList coreCats = coreByCategory.keys();
    coreCats.sort();
    bool coreHasChildren = false;

    for (const auto &cat : coreCats) {
        auto *catItem = new QTreeWidgetItem(coreRoot, {cat});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
        QPixmap px(12, 12); px.fill(categoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        for (const auto *mod : coreByCategory[cat]) {
            if (addModuleItem(catItem, mod, m_languageFilter))
                catHasChildren = true;
        }
        catItem->setHidden(!catHasChildren);
        if (catHasChildren) coreHasChildren = true;
    }
    coreRoot->setHidden(!coreHasChildren);
    coreRoot->setExpanded(true);

    // === Секция 2: UI-виджеты ===
    auto *uiRoot = new QTreeWidgetItem(m_tree, {tr("UI-виджеты")});
    uiRoot->setFlags(uiRoot->flags() & ~Qt::ItemIsDragEnabled);
    uiRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(220, 80, 80));
        uiRoot->setIcon(0, QIcon(px));
    }

    bool uiHasChildren = false;
    for (const auto *mod : allModules) {
        if (mod->origin == "ui") {
            if (addModuleItem(uiRoot, mod, m_languageFilter))
                uiHasChildren = true;
        }
    }
    uiRoot->setHidden(!uiHasChildren);
    uiRoot->setExpanded(true);

    // === Секция 3: Расширения (extension) ===
    auto *extRoot = new QTreeWidgetItem(m_tree, {tr("Расширения")});
    extRoot->setFlags(extRoot->flags() & ~Qt::ItemIsDragEnabled);
    extRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(80, 180, 80));
        extRoot->setIcon(0, QIcon(px));
    }

    // Собираем extension-модули по категориям
    QMap<QString, QVector<const Module *>> extByCategory;
    for (const auto *mod : allModules) {
        if (mod->origin == "extension")
            extByCategory[mod->category].append(mod);
    }

    bool extHasChildren = false;
    QStringList extCats = extByCategory.keys();
    extCats.sort();
    for (const auto &cat : extCats) {
        auto *catItem = new QTreeWidgetItem(extRoot, {cat});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
        QPixmap px(12, 12); px.fill(categoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        for (const auto *mod : extByCategory[cat]) {
            if (addModuleItem(catItem, mod, m_languageFilter))
                catHasChildren = true;
        }
        catItem->setHidden(!catHasChildren);
        if (catHasChildren) extHasChildren = true;
    }

    // Также показываем модули без категории (user/library/graph)
    for (const auto *mod : allModules) {
        if (mod->origin != "core" && mod->origin != "ui" && mod->origin != "extension") {
            if (!mod->category.isEmpty()) {
                // Ищем существующую подкатегорию или создаём
                bool found = false;
                for (int i = 0; i < extRoot->childCount(); ++i) {
                    if (extRoot->child(i)->text(0) == mod->category) {
                        if (addModuleItem(extRoot->child(i), mod, m_languageFilter))
                            extHasChildren = true;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    auto *catItem = new QTreeWidgetItem(extRoot, {mod->category});
                    catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
                    QPixmap px(12, 12); px.fill(categoryColor(mod->category));
                    catItem->setIcon(0, QIcon(px));
                    if (addModuleItem(catItem, mod, m_languageFilter))
                        extHasChildren = true;
                }
            } else {
                if (addModuleItem(extRoot, mod, m_languageFilter))
                    extHasChildren = true;
            }
        }
    }

    extRoot->setHidden(!extHasChildren);
    extRoot->setExpanded(true);

    // === Секция 4: Проектные модули (local) ===
    auto *localRoot = new QTreeWidgetItem(m_tree, {tr("Проектные модули")});
    localRoot->setFlags(localRoot->flags() & ~Qt::ItemIsDragEnabled);
    localRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(180, 140, 50));
        localRoot->setIcon(0, QIcon(px));
    }

    bool localHasChildren = false;
    for (const auto *mod : allModules) {
        if (mod->origin == "local" || mod->origin == "graph") {
            if (addModuleItem(localRoot, mod, m_languageFilter))
                localHasChildren = true;
        }
    }
    localRoot->setHidden(!localHasChildren);
    localRoot->setExpanded(true);
}

void ModulePalette::filterTree(const QString &text)
{
    // Рекурсивно фильтруем 3-уровневое дерево (секция → категория → модуль)
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *sectionItem = m_tree->topLevelItem(i);
        bool sectionVisible = false;

        for (int j = 0; j < sectionItem->childCount(); ++j) {
            auto *child = sectionItem->child(j);

            if (child->childCount() > 0) {
                // Это подкатегория — фильтруем её детей
                bool catVisible = false;
                for (int k = 0; k < child->childCount(); ++k) {
                    auto *modItem = child->child(k);
                    bool match = text.isEmpty() ||
                                 modItem->text(0).contains(text, Qt::CaseInsensitive);
                    modItem->setHidden(!match);
                    if (match) catVisible = true;
                }
                child->setHidden(!catVisible);
                if (catVisible) sectionVisible = true;
            } else {
                // Это модуль напрямую в секции
                bool match = text.isEmpty() ||
                             child->text(0).contains(text, Qt::CaseInsensitive);
                child->setHidden(!match);
                if (match) sectionVisible = true;
            }
        }

        sectionItem->setHidden(!sectionVisible);
    }
}

void ModulePalette::startDragForItem(QTreeWidgetItem *item)
{
    if (!item) return;

    // Только листовые элементы с moduleId (не секции и не категории)
    QString moduleId = item->data(0, Qt::UserRole).toString();
    if (moduleId.isEmpty()) return;

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setData("application/x-dqmodule", moduleId.toUtf8());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

} // namespace DeltaQ

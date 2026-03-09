// Палитра модулей — реализация дерева категорий, поиска и drag&drop
#include "ModulePalette.h"
#include "../core/ModuleRegistry.h"
#include "../core/StandardLibrary.h"
#include <deltaq/Module.h>

#include <QHeaderView>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QMouseEvent>
#include <QCheckBox>

namespace DeltaQ {

namespace {

// Человекочитаемый label imported-модуля для палитры: display name плюс роль, если это raw/adapter.
QString paletteModuleLabel(const Module &module)
{
    if (!module.isImportedPackModule()) {
        QString label = module.name;
        if ((module.origin == "core" || module.id.startsWith("core."))) {
            // Legacy core-модуль должен быть сразу заметен в рабочей палитре,
            // чтобы пользователь не принимал его за рекомендуемый baseline.
            const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(module);
            if (curation.tier == "legacy")
                label += QObject::tr(" [legacy]");
        }
        return label;
    }

    QString label = module.importedDisplayName();
    const QString role = module.importedCurationRole();
    if (role == "raw_wrapper")
        label += QObject::tr(" [raw]");
    else if (role == "adapter")
        label += QObject::tr(" [adapter]");
    return label;
}

// Формирует блок tooltip для imported pack metadata, чтобы curation-flow был виден прямо в палитре.
QString importedModuleTooltip(const Module &module)
{
    if (!module.isImportedPackModule())
        return {};

    QString roleTitle = QObject::tr("curated");
    const QString role = module.importedCurationRole();
    if (role == "raw_wrapper")
        roleTitle = QObject::tr("raw");
    else if (role == "adapter")
        roleTitle = QObject::tr("adapter");
    else if (role == "hidden")
        roleTitle = QObject::tr("hidden");

    return QObject::tr("\n\nImported pack: %1\nСимвол: %2\nРоль import/curation: %3")
        .arg(module.importedPackName(),
             module.importedOriginalSymbol(),
             roleTitle);
}

// Добавляет к tooltip стандартизованный блок документации модуля.
void appendDocumentationTooltip(QString &tip, const Module &module)
{
    if (!module.hasDocumentationDetails())
        return;

    const QString summary = module.documentationSummary();
    const QString whenToUse = module.documentationWhenToUse();
    const QString limitations = module.documentationLimitations();

    if (!summary.isEmpty())
        tip += QObject::tr("Назначение: %1").arg(summary);
    if (!whenToUse.isEmpty()) {
        if (!tip.isEmpty())
            tip += '\n';
        tip += QObject::tr("Когда использовать: %1").arg(whenToUse);
    }
    if (!limitations.isEmpty()) {
        if (!tip.isEmpty())
            tip += '\n';
        tip += QObject::tr("Ограничения: %1").arg(limitations);
    }
}

// Сводка imported pack-а в дереве, чтобы pack boundary читался без открытия docs.
QString importedPackTooltip(const QString &packName, const QVector<const Module *> &modules)
{
    int rawCount = 0;
    int curatedCount = 0;
    int adapterCount = 0;
    int hiddenCount = 0;

    for (const auto *module : modules) {
        const QString role = module->importedCurationRole();
        if (role == "raw_wrapper")
            ++rawCount;
        else if (role == "adapter")
            ++adapterCount;
        else if (role == "hidden")
            ++hiddenCount;
        else
            ++curatedCount;
    }

    return QObject::tr("Imported pack: %1\nCurated: %2 | Raw: %3 | Adapter: %4 | Hidden: %5")
        .arg(packName)
        .arg(curatedCount)
        .arg(rawCount)
        .arg(adapterCount)
        .arg(hiddenCount);
}

// Рекурсивный фильтр дерева: поддерживает произвольную глубину (section -> pack -> category -> module).
bool filterTreeItemRecursive(QTreeWidgetItem *item, const QString &text, bool ancestorMatched = false)
{
    if (!item)
        return false;

    const bool selfMatch = ancestorMatched
        || text.isEmpty()
        || item->text(0).contains(text, Qt::CaseInsensitive);

    if (item->childCount() == 0) {
        item->setHidden(!selfMatch);
        return selfMatch;
    }

    bool childVisible = false;
    for (int i = 0; i < item->childCount(); ++i) {
        if (filterTreeItemRecursive(item->child(i), text, selfMatch))
            childVisible = true;
    }

    const bool visible = selfMatch || childVisible;
    item->setHidden(!visible);
    return visible;
}

} // namespace

ModulePalette::ModulePalette(ModuleRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search modules..."));
    layout->addWidget(m_searchEdit);

    m_showSpecializedCheck = new QCheckBox(tr("Показывать specialized"), this);
    m_showSpecializedCheck->setObjectName("modulePaletteShowSpecializedCheck");
    m_showSpecializedCheck->setChecked(false);
    layout->addWidget(m_showSpecializedCheck);

    // Legacy-модули не должны навязываться в рабочем baseline,
    // но пользователь может явно вернуть их в палитру для совместимости.
    m_showLegacyCheck = new QCheckBox(tr("Показывать legacy"), this);
    m_showLegacyCheck->setObjectName("modulePaletteShowLegacyCheck");
    m_showLegacyCheck->setChecked(false);
    layout->addWidget(m_showLegacyCheck);

    m_tree = new ModuleTreeWidget(this);
    m_tree->setObjectName("modulePaletteTree");
    m_tree->setHeaderHidden(true);
    m_tree->setDragEnabled(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tree);

    // Фильтрация при вводе
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ModulePalette::filterTree);
    connect(m_showSpecializedCheck, &QCheckBox::toggled, this, [this] {
        rebuildTree();
        filterTree(m_searchEdit->text());
    });
    connect(m_showLegacyCheck, &QCheckBox::toggled, this, [this] {
        rebuildTree();
        filterTree(m_searchEdit->text());
    });

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
    QString tip;
    appendDocumentationTooltip(tip, *mod);
    tip += importedModuleTooltip(*mod);
    if (mod->origin == "core" || mod->id.startsWith("core.")) {
        const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(*mod);
        if (curation.isKnown()) {
            tip += QObject::tr("\n\nРоль в библиотеке: %1").arg(curation.title);
            if (!curation.guidance.isEmpty())
                tip += QObject::tr("\nПодсказка: %1").arg(curation.guidance);
            if (!curation.replacementHint.isEmpty())
                tip += QObject::tr("\nАльтернатива: %1").arg(curation.replacementHint);
        }
    }
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

// Добавляет к модулю видимый и tooltip-статус допуска в граф.
static QString moduleAdmissionSuffix(ModuleRegistry *registry, const Module *mod, QString *tooltip)
{
    QString reason;
    if (!registry || registry->isModuleAdmittedForComposition(*mod, &reason)) {
        if (tooltip)
            *tooltip += QObject::tr("\n\nСтатус: готов к использованию в графе");
        return {};
    }

    if (tooltip) {
        *tooltip += QObject::tr("\n\nСтатус: не готов к использованию в графе");
        if (!reason.isEmpty())
            *tooltip += QObject::tr("\nПричина: %1").arg(reason);
    }
    return QObject::tr(" [не готов]");
}

// Добавить модуль в дерево с проверкой языка
QTreeWidgetItem *addModuleItem(QTreeWidgetItem *parent, const Module *mod,
                                const QString &languageFilter,
                                ModuleRegistry *registry,
                                bool showSpecializedModules,
                                bool showLegacyModules)
{
    // Hidden imported wrapper-ы остаются в Module Manager, но не засоряют рабочую палитру.
    if (mod->isHiddenFromPalette())
        return nullptr;

    if (mod->origin == "core" || mod->id.startsWith("core.")) {
        const StandardLibraryCurationInfo curation = StandardLibrary::curationForModule(*mod);
        // Базовая палитра должна показывать рекомендуемое ядро библиотеки.
        // Specialized и legacy остаются доступными, но только по явному запросу.
        if (curation.tier == "specialized" && !showSpecializedModules)
            return nullptr;
        if (curation.tier == "legacy" && !showLegacyModules)
            return nullptr;
    }

    if (!languageFilter.isEmpty()) {
        bool compatible = (mod->language == languageFilter) ||
            (languageFilter == "c" && mod->language == "cpp") ||
            (languageFilter == "cpp" && mod->language == "c") ||
            mod->language.isEmpty();
        if (!compatible) return nullptr;
    }

    QString tooltip = moduleTooltip(mod);
    const QString suffix = moduleAdmissionSuffix(registry, mod, &tooltip);
    auto *item = new QTreeWidgetItem(parent, {paletteModuleLabel(*mod) + suffix});
    item->setData(0, Qt::UserRole, mod->id);
    item->setToolTip(0, tooltip);

    QString reason;
    if (registry && registry->isModuleAdmittedForComposition(*mod, &reason)) {
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    } else {
        item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
        item->setForeground(0, QColor(150, 150, 150));
    }
    return item;
}

void ModulePalette::buildTree()
{
    m_tree->clear();

    if (!m_registry) return;

    auto allModules = m_registry->allModules();
    const bool showSpecializedModules = m_showSpecializedCheck && m_showSpecializedCheck->isChecked();
    const bool showLegacyModules = m_showLegacyCheck && m_showLegacyCheck->isChecked();

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

    // Палитра standard library должна читаться так же, как и в Module Manager:
    // сначала product-defined v1 категории, затем возможный алфавитный хвост.
    const QStringList coreCats = StandardLibrary::orderCategoriesForDisplay(coreByCategory.keys());
    bool coreHasChildren = false;

    for (const auto &cat : coreCats) {
        auto *catItem = new QTreeWidgetItem(coreRoot, {cat});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
        QPixmap px(12, 12); px.fill(categoryColor(cat));
        catItem->setIcon(0, QIcon(px));

        bool catHasChildren = false;
        const QVector<const Module *> orderedModules =
            StandardLibrary::orderModulesForDisplay(coreByCategory[cat]);
        for (const auto *mod : orderedModules) {
            if (addModuleItem(catItem, mod, m_languageFilter, m_registry,
                              showSpecializedModules, showLegacyModules))
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
            if (addModuleItem(uiRoot, mod, m_languageFilter, m_registry, true, showLegacyModules))
                uiHasChildren = true;
        }
    }
    uiRoot->setHidden(!uiHasChildren);
    uiRoot->setExpanded(true);

    // === Секция 3: Imported pack-и ===
    auto *importedRoot = new QTreeWidgetItem(m_tree, {tr("Импортированные пакеты")});
    importedRoot->setFlags(importedRoot->flags() & ~Qt::ItemIsDragEnabled);
    importedRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(90, 170, 170));
        importedRoot->setIcon(0, QIcon(px));
    }

    QMap<QString, QVector<const Module *>> importedByPack;
    for (const auto *mod : allModules) {
        if (mod->isImportedPackModule())
            importedByPack[mod->importedPackName()].append(mod);
    }

    bool importedHasChildren = false;
    QStringList packNames = importedByPack.keys();
    packNames.sort();
    for (const auto &packName : packNames) {
        auto *packItem = new QTreeWidgetItem(importedRoot, {packName});
        packItem->setFlags(packItem->flags() & ~Qt::ItemIsDragEnabled);
        packItem->setToolTip(0, importedPackTooltip(packName, importedByPack[packName]));

        QMap<QString, QVector<const Module *>> packByCategory;
        for (const auto *mod : importedByPack[packName]) {
            const QString category = mod->category.isEmpty() ? QString("custom") : mod->category;
            packByCategory[category].append(mod);
        }

        bool packHasChildren = false;
        QStringList packCats = packByCategory.keys();
        packCats.sort();
        for (const auto &cat : packCats) {
            auto *catItem = new QTreeWidgetItem(packItem, {cat});
            catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
            QPixmap px(12, 12); px.fill(categoryColor(cat));
            catItem->setIcon(0, QIcon(px));

            bool catHasChildren = false;
            for (const auto *mod : packByCategory[cat]) {
                if (addModuleItem(catItem, mod, m_languageFilter, m_registry,
                                  true, showLegacyModules)) {
                    catHasChildren = true;
                }
            }
            catItem->setHidden(!catHasChildren);
            if (catHasChildren)
                packHasChildren = true;
        }

        packItem->setHidden(!packHasChildren);
        packItem->setExpanded(true);
        if (packHasChildren)
            importedHasChildren = true;
    }
    importedRoot->setHidden(!importedHasChildren);
    importedRoot->setExpanded(true);

    // === Секция 4: Расширения (extension) ===
    auto *extRoot = new QTreeWidgetItem(m_tree, {tr("Расширения")});
    extRoot->setFlags(extRoot->flags() & ~Qt::ItemIsDragEnabled);
    extRoot->setFont(0, boldFont);
    {
        QPixmap px(12, 12); px.fill(QColor(80, 180, 80));
        extRoot->setIcon(0, QIcon(px));
    }

    QMap<QString, QVector<const Module *>> extByCategory;
    for (const auto *mod : allModules) {
        if (mod->origin == "extension" && !mod->isImportedPackModule()) {
            const QString category = mod->category.isEmpty() ? QString("custom") : mod->category;
            extByCategory[category].append(mod);
        }
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
            if (addModuleItem(catItem, mod, m_languageFilter, m_registry, true, showLegacyModules))
                catHasChildren = true;
        }
        catItem->setHidden(!catHasChildren);
        if (catHasChildren)
            extHasChildren = true;
    }

    // Также показываем обычные внешние модули без pack metadata.
    for (const auto *mod : allModules) {
        if (mod->origin == "core" || mod->origin == "ui" || mod->origin == "extension"
            || mod->origin == "local" || mod->origin == "graph"
            || mod->isImportedPackModule()) {
            continue;
        }

        if (!mod->category.isEmpty()) {
            bool found = false;
            for (int i = 0; i < extRoot->childCount(); ++i) {
                if (extRoot->child(i)->text(0) == mod->category) {
                    if (addModuleItem(extRoot->child(i), mod, m_languageFilter, m_registry,
                                      true, showLegacyModules)) {
                        extHasChildren = true;
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                auto *catItem = new QTreeWidgetItem(extRoot, {mod->category});
                catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);
                QPixmap px(12, 12); px.fill(categoryColor(mod->category));
                catItem->setIcon(0, QIcon(px));
                if (addModuleItem(catItem, mod, m_languageFilter, m_registry, true, showLegacyModules))
                    extHasChildren = true;
            }
        } else {
            if (addModuleItem(extRoot, mod, m_languageFilter, m_registry, true, showLegacyModules))
                extHasChildren = true;
        }
    }

    extRoot->setHidden(!extHasChildren);
    extRoot->setExpanded(true);

    // === Секция 5: Проектные модули (local) ===
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
            if (addModuleItem(localRoot, mod, m_languageFilter, m_registry, true, showLegacyModules))
                localHasChildren = true;
        }
    }
    localRoot->setHidden(!localHasChildren);
    localRoot->setExpanded(true);
}

void ModulePalette::filterTree(const QString &text)
{
    // Рекурсивный фильтр поддерживает и старую структуру, и новую pack-группировку.
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        filterTreeItemRecursive(m_tree->topLevelItem(i), text);
    }
}

void ModulePalette::startDragForItem(QTreeWidgetItem *item)
{
    if (!item) return;

    // Только листовые элементы с moduleId (не секции и не категории)
    QString moduleId = item->data(0, Qt::UserRole).toString();
    if (moduleId.isEmpty()) return;

    const Module *mod = m_registry ? m_registry->findModule(moduleId) : nullptr;
    QString reason;
    if (!mod || !m_registry->isModuleAdmittedForComposition(*mod, &reason))
        return;

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setData("application/x-dqmodule", moduleId.toUtf8());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

} // namespace DeltaQ

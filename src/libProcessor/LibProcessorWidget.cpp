// Модуль 4: Обработчик библиотек — реализация
#include "LibProcessorWidget.h"
#include "LibraryImportWizard.h"
#include "../core/ModuleRegistry.h"

#include <QSplitter>
#include <QToolBar>
#include <QTreeWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QAction>
#include <QFont>

namespace DeltaQ {

LibProcessorWidget::LibProcessorWidget(ModuleRegistry *registry, QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
{
    setupUI();

    // Обновление при регистрации модулей
    if (m_registry) {
        connect(m_registry, &ModuleRegistry::moduleRegistered, this,
                &LibProcessorWidget::refreshLibraryTree);
        connect(m_registry, &ModuleRegistry::moduleUnregistered, this,
                &LibProcessorWidget::refreshLibraryTree);
    }
}

void LibProcessorWidget::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupToolBar();
    mainLayout->addWidget(m_toolbar);

#ifndef DQ_HAS_LIBCLANG
    // Предупреждение если libclang недоступен
    auto *warning = new QLabel(
        tr("libclang not found.\n\n"
           "Library import requires libclang.\n"
           "Install libclang-dev and rebuild with -DDQ_USE_LIBCLANG=ON.\n\n"
           "On Ubuntu/Debian: sudo apt install libclang-dev\n"
           "On Fedora: sudo dnf install clang-devel"),
        this);
    warning->setAlignment(Qt::AlignCenter);
    warning->setWordWrap(true);
    warning->setStyleSheet("color: #e0a020; padding: 20px; font-size: 13px;");
    mainLayout->addWidget(warning);
#endif

    // Splitter: дерево библиотек | детали модуля
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_libraryTree = new QTreeWidget;
    m_libraryTree->setHeaderLabels({tr("Module"), tr("Category"), tr("Origin")});
    m_libraryTree->header()->setStretchLastSection(true);
    m_libraryTree->setRootIsDecorated(true);
    connect(m_libraryTree, &QTreeWidget::currentItemChanged, this, [this]() {
        onModuleSelected();
    });
    m_splitter->addWidget(m_libraryTree);

    m_moduleDetails = new QTextEdit;
    m_moduleDetails->setReadOnly(true);
    m_moduleDetails->setFont(QFont("Monospace", 10));
    m_moduleDetails->setPlaceholderText(tr("Select a module to view details"));
    m_splitter->addWidget(m_moduleDetails);

    m_splitter->setSizes({400, 400});
    mainLayout->addWidget(m_splitter);

    refreshLibraryTree();
}

void LibProcessorWidget::setupToolBar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));

    auto *importAction = m_toolbar->addAction(tr("Import Library"));
    connect(importAction, &QAction::triggered, this, &LibProcessorWidget::onImportLibrary);

    m_toolbar->addSeparator();
    m_toolbar->addAction(tr("Refresh"), this, &LibProcessorWidget::refreshLibraryTree);
}

void LibProcessorWidget::onImportLibrary()
{
    auto *wizard = new LibraryImportWizard(m_registry, this);
    wizard->exec();
    wizard->deleteLater();
    refreshLibraryTree();
}

void LibProcessorWidget::onModuleSelected()
{
    auto *item = m_libraryTree->currentItem();
    if (!item || !m_registry) {
        m_moduleDetails->clear();
        return;
    }

    QString moduleId = item->data(0, Qt::UserRole).toString();
    const Module *mod = m_registry->findModule(moduleId);
    if (!mod) {
        m_moduleDetails->clear();
        return;
    }

    QString details;
    details += QString("Name: %1\n").arg(mod->name);
    details += QString("ID: %1\n").arg(mod->id);
    details += QString("Version: %1\n").arg(mod->version);
    details += QString("Language: %1\n").arg(mod->language);
    details += QString("Category: %1\n").arg(mod->category);
    details += QString("Origin: %1\n").arg(mod->origin);
    details += QString("Description: %1\n").arg(mod->description);

    const QString importedPack = mod->metadataString("deltaq.import.pack_name");
    if (!importedPack.isEmpty()) {
        details += QString("Imported Pack: %1\n").arg(importedPack);
        details += QString("Original Symbol: %1\n")
            .arg(mod->metadataString("deltaq.import.original_symbol"));
        details += QString("Standard: %1\n")
            .arg(mod->metadataString("deltaq.import.standard"));
    }

    if (!mod->inputs.isEmpty()) {
        details += "\nInputs:\n";
        for (const auto &p : mod->inputs) {
            details += QString("  %1 : %2").arg(p.name, p.type);
            if (!p.defaultValue.isEmpty())
                details += QString(" = %1").arg(p.defaultValue);
            details += "\n";
        }
    }

    if (!mod->outputs.isEmpty()) {
        details += "\nOutputs:\n";
        for (const auto &p : mod->outputs)
            details += QString("  %1 : %2\n").arg(p.name, p.type);
    }

    m_moduleDetails->setPlainText(details);
}

void LibProcessorWidget::refreshLibraryTree()
{
    if (!m_libraryTree || !m_registry) return;

    m_libraryTree->clear();

    // Группируем imported modules по pack-у, чтобы Library Processor показывал уже
    // не временные контракты, а реальные extension pack-ы.
    QMap<QString, QTreeWidgetItem *> packItems;
    QMap<QString, QTreeWidgetItem *> categoryItems;

    auto modules = m_registry->allModules();
    for (const auto *mod : modules) {
        const QString importedPack = mod->metadataString("deltaq.import.pack_name");
        if (mod->origin != "library" && importedPack.isEmpty())
            continue;

        const QString packKey = importedPack.isEmpty() ? tr("Transient Imports") : importedPack;
        if (!packItems.contains(packKey)) {
            auto *packItem = new QTreeWidgetItem(m_libraryTree, {packKey, "", ""});
            packItem->setFlags(packItem->flags() & ~Qt::ItemIsSelectable);
            packItems[packKey] = packItem;
        }

        const QString cat = mod->category.isEmpty() ? tr("Uncategorized") : mod->category;
        const QString categoryKey = packKey + "::" + cat;
        if (!categoryItems.contains(categoryKey)) {
            auto *catItem = new QTreeWidgetItem(packItems[packKey], {cat, "", ""});
            catItem->setFlags(catItem->flags() & ~Qt::ItemIsSelectable);
            categoryItems[categoryKey] = catItem;
        }

        auto *item = new QTreeWidgetItem(categoryItems[categoryKey], {
            mod->name, mod->category, mod->origin
        });
        item->setData(0, Qt::UserRole, mod->id);
    }

    m_libraryTree->expandAll();
}

} // namespace DeltaQ

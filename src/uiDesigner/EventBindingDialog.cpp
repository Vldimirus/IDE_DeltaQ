// Диалог привязки событий — реализация
#include "EventBindingDialog.h"

#include "../core/ModuleRegistry.h"
#include "../core/GraphStore.h"
#include <deltaq/Module.h>
#include <deltaq/Graph.h>

#include <QTabWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>

namespace DeltaQ {

EventBindingDialog::EventBindingDialog(ModuleRegistry *registry, GraphStore *graphStore,
                                         QWidget *parent)
    : QDialog(parent)
    , m_registry(registry)
    , m_graphStore(graphStore)
{
    setWindowTitle(tr("Bind Event"));
    setMinimumSize(500, 400);

    auto *layout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    layout->addWidget(m_tabWidget);

    buildModuleFunctionTab();
    buildGraphTriggerTab();
    buildCustomFunctionTab();

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        int tab = m_tabWidget->currentIndex();
        if (tab == 0) {
            // Module function
            auto *item = m_moduleFuncTree->currentItem();
            if (item && item->parent())
                m_selectedHandler = item->data(0, Qt::UserRole).toString();
        } else if (tab == 1) {
            // Graph trigger
            auto *item = m_graphTriggerTree->currentItem();
            if (item)
                m_selectedHandler = "graph:" + item->data(0, Qt::UserRole).toString();
        } else {
            // Custom function
            m_selectedHandler = m_customFuncEdit->text();
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void EventBindingDialog::setEventName(const QString &name)
{
    m_eventName = name;
}

void EventBindingDialog::setCurrentHandler(const QString &handler)
{
    m_selectedHandler = handler;
}

void EventBindingDialog::buildModuleFunctionTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *label = new QLabel(tr("Select a module function:"), widget);
    layout->addWidget(label);

    m_moduleFuncTree = new QTreeWidget(widget);
    m_moduleFuncTree->setHeaderLabels({tr("Function"), tr("Type")});
    m_moduleFuncTree->header()->setStretchLastSection(true);
    layout->addWidget(m_moduleFuncTree);

    // Заполняем из ModuleRegistry
    if (m_registry) {
        auto modules = m_registry->allModules();
        for (const auto *mod : modules) {
            auto *modItem = new QTreeWidgetItem(m_moduleFuncTree, {mod->name, mod->category});
            modItem->setFlags(modItem->flags() & ~Qt::ItemIsSelectable);

            // Каждый модуль = вызываемая функция
            auto *funcItem = new QTreeWidgetItem(modItem, {mod->name + "()", "function"});
            funcItem->setData(0, Qt::UserRole, mod->id);
        }
    }

    m_moduleFuncTree->expandAll();
    m_tabWidget->addTab(widget, tr("Module Function"));
}

void EventBindingDialog::buildGraphTriggerTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *label = new QLabel(tr("Select a graph to trigger:"), widget);
    layout->addWidget(label);

    m_graphTriggerTree = new QTreeWidget(widget);
    m_graphTriggerTree->setHeaderLabels({tr("Graph"), tr("Nodes")});
    layout->addWidget(m_graphTriggerTree);

    // Заполняем из GraphStore
    if (m_graphStore) {
        auto graphs = m_graphStore->allGraphs();
        for (const auto *graph : graphs) {
            auto *item = new QTreeWidgetItem(m_graphTriggerTree, {
                graph->name,
                QString::number(graph->nodeCount())
            });
            item->setData(0, Qt::UserRole, graph->id);
        }
    }

    m_tabWidget->addTab(widget, tr("Graph Trigger"));
}

void EventBindingDialog::buildCustomFunctionTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *label = new QLabel(tr("Enter custom function name:"), widget);
    layout->addWidget(label);

    m_customFuncEdit = new QLineEdit(widget);
    m_customFuncEdit->setPlaceholderText("on_button_click");
    layout->addWidget(m_customFuncEdit);

    layout->addStretch();

    m_tabWidget->addTab(widget, tr("Custom Function"));
}

} // namespace DeltaQ

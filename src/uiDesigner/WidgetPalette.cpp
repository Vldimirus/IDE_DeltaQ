// Палитра виджетов — реализация дерева категорий, поиска и drag&drop
#include "WidgetPalette.h"

#include <QHeaderView>
#include <QDrag>
#include <QMimeData>

namespace DeltaQ {

// Описание виджетов по категориям
struct WidgetTypeInfo {
    QString type;
    QString displayName;
};

struct WidgetCategory {
    QString name;
    QColor color;
    QVector<WidgetTypeInfo> widgets;
};

static QVector<WidgetCategory> widgetCategories()
{
    return {
        {"Containers", QColor(80, 130, 200), {
            {"Panel",       "Panel"},
            {"ScrollPanel", "Scroll Panel"},
            {"TabPanel",    "Tab Panel"},
            {"GroupBox",    "Group Box"},
        }},
        {"Input", QColor(200, 140, 50), {
            {"Button",      "Button"},
            {"TextField",   "Text Field"},
            {"TextArea",    "Text Area"},
            {"Checkbox",    "Checkbox"},
            {"RadioButton", "Radio Button"},
            {"ComboBox",    "Combo Box"},
            {"Slider",      "Slider"},
            {"SpinBox",     "Spin Box"},
        }},
        {"Display", QColor(80, 170, 80), {
            {"Label",       "Label"},
            {"Image",       "Image"},
            {"ProgressBar", "Progress Bar"},
            {"Canvas",      "Canvas"},
            {"Table",       "Table"},
            {"ListView",    "List View"},
            {"TreeView",    "Tree View"},
        }},
        {"Navigation", QColor(170, 80, 170), {
            {"MenuBar",   "Menu Bar"},
            {"ToolBar",   "Tool Bar"},
            {"StatusBar", "Status Bar"},
        }},
    };
}

WidgetPalette::WidgetPalette(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search widgets..."));
    layout->addWidget(m_searchEdit);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setDragEnabled(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setDragDropMode(QAbstractItemView::DragOnly);
    layout->addWidget(m_tree);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &WidgetPalette::filterTree);

    buildTree();
}

void WidgetPalette::setFilter(const QString &text)
{
    m_searchEdit->setText(text);
}

void WidgetPalette::buildTree()
{
    m_tree->clear();

    auto cats = widgetCategories();
    for (const auto &cat : cats) {
        auto *catItem = new QTreeWidgetItem(m_tree, {cat.name});
        catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);

        QPixmap px(12, 12);
        px.fill(cat.color);
        catItem->setIcon(0, QIcon(px));

        for (const auto &w : cat.widgets) {
            auto *wItem = new QTreeWidgetItem(catItem, {w.displayName});
            wItem->setData(0, Qt::UserRole, w.type);
            wItem->setFlags(wItem->flags() | Qt::ItemIsDragEnabled);
            wItem->setToolTip(0, tr("Drag to add %1").arg(w.displayName));
        }

        catItem->setExpanded(true);
    }

    // Drag из дерева
    connect(m_tree, &QTreeWidget::itemPressed, this, [this](QTreeWidgetItem *item, int) {
        startDragForItem(item);
    }, Qt::UniqueConnection);
}

void WidgetPalette::filterTree(const QString &text)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto *catItem = m_tree->topLevelItem(i);
        bool catVisible = false;

        for (int j = 0; j < catItem->childCount(); ++j) {
            auto *wItem = catItem->child(j);
            bool match = text.isEmpty() ||
                         wItem->text(0).contains(text, Qt::CaseInsensitive);
            wItem->setHidden(!match);
            if (match) catVisible = true;
        }

        catItem->setHidden(!catVisible);
    }
}

void WidgetPalette::startDragForItem(QTreeWidgetItem *item)
{
    if (!item || !item->parent()) return;

    QString widgetType = item->data(0, Qt::UserRole).toString();
    if (widgetType.isEmpty()) return;

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setData("application/x-dqwidget", widgetType.toUtf8());
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

} // namespace DeltaQ

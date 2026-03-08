// Палитра виджетов — реализация дерева категорий, поиска и drag&drop
#include "WidgetPalette.h"

#include <deltaq/UIContract.h>

#include <QHeaderView>

namespace DeltaQ {

// Цвет категории берётся из фиксированной палитры, а сами виджеты уже читаются из UIContract.
static QColor paletteCategoryColor(const QString &category)
{
    if (category == "Containers")
        return QColor(80, 130, 200);
    if (category == "Input")
        return QColor(200, 140, 50);
    if (category == "Display")
        return QColor(80, 170, 80);
    if (category == "Navigation")
        return QColor(170, 80, 170);
    return QColor(110, 110, 110);
}

WidgetPalette::WidgetPalette(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search widgets..."));
    layout->addWidget(m_searchEdit);

    m_tree = new PaletteTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    // Встроенный drag Qt — PaletteTreeWidget::mimeData() даёт правильный MIME
    m_tree->setDragEnabled(true);
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

    QMap<QString, QTreeWidgetItem *> categoryItems;
    for (const auto &spec : uiContractCatalog()) {
        if (!spec.showInPalette)
            continue;

        QTreeWidgetItem *catItem = categoryItems.value(spec.paletteCategory);
        if (!catItem) {
            catItem = new QTreeWidgetItem(m_tree, {spec.paletteCategory});
            catItem->setFlags(catItem->flags() & ~Qt::ItemIsDragEnabled);

            QPixmap px(12, 12);
            px.fill(paletteCategoryColor(spec.paletteCategory));
            catItem->setIcon(0, QIcon(px));
            catItem->setExpanded(true);
            categoryItems.insert(spec.paletteCategory, catItem);
        }

        auto *wItem = new QTreeWidgetItem(catItem, {spec.displayName});
        wItem->setData(0, Qt::UserRole, spec.legacyWidgetType);
        wItem->setToolTip(0, tr("Drag to add %1").arg(spec.displayName));
    }
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

} // namespace DeltaQ

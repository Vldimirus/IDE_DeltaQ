// Палитра виджетов — боковая панель с деревом категорий и поиском
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace DeltaQ {

class WidgetPalette : public QWidget {
    Q_OBJECT

public:
    explicit WidgetPalette(QWidget *parent = nullptr);

    void setFilter(const QString &text);

private:
    void buildTree();
    void filterTree(const QString &text);
    void startDragForItem(QTreeWidgetItem *item);

    QLineEdit *m_searchEdit = nullptr;
    QTreeWidget *m_tree = nullptr;
};

} // namespace DeltaQ

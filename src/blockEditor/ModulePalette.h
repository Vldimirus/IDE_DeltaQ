// Палитра модулей — боковая панель с деревом категорий и поиском
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace DeltaQ {

class ModuleRegistry;

class ModulePalette : public QWidget {
    Q_OBJECT

public:
    explicit ModulePalette(ModuleRegistry *registry, QWidget *parent = nullptr);

    // Перестроить дерево из реестра
    void rebuildTree();

    // Фильтрация по поисковой строке
    void setFilter(const QString &text);

    // Фильтрация по языку проекта
    void setLanguageFilter(const QString &lang);

private:
    void buildTree();
    void filterTree(const QString &text);
    void startDragForItem(QTreeWidgetItem *item);

    ModuleRegistry *m_registry;
    QLineEdit *m_searchEdit = nullptr;
    QTreeWidget *m_tree = nullptr;
    QString m_languageFilter; // "" = все, "c", "cpp"
};

} // namespace DeltaQ

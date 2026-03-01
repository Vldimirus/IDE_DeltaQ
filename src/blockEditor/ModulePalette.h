// Палитра модулей — боковая панель с деревом категорий и поиском
#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeWidget>
#include <QMimeData>
#include <QVBoxLayout>

namespace DeltaQ {

class ModuleRegistry;

// Дерево с правильным MIME-типом для drag&drop модулей
class ModuleTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    using QTreeWidget::QTreeWidget;

protected:
    QStringList mimeTypes() const override {
        return {"application/x-dqmodule"};
    }

    QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override {
        if (items.isEmpty()) return nullptr;
        QString moduleId = items.first()->data(0, Qt::UserRole).toString();
        if (moduleId.isEmpty()) return nullptr;
        auto *data = new QMimeData;
        data->setData("application/x-dqmodule", moduleId.toUtf8());
        return data;
    }
};

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
    ModuleTreeWidget *m_tree = nullptr;
    QString m_languageFilter; // "" = все, "c", "cpp"
};

} // namespace DeltaQ

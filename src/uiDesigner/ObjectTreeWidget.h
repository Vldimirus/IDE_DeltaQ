// Дерево объектов UI — иерархическое отображение виджетов на сцене
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace DeltaQ {

class DesignScene;
class WidgetItem;

class ObjectTreeWidget : public QWidget {
    Q_OBJECT

public:
    explicit ObjectTreeWidget(QWidget *parent = nullptr);

    // Перестроить дерево из текущего состояния сцены
    void rebuild(DesignScene *scene);

    // Выделить элемент по widgetId (без эмиссии сигнала)
    void selectWidget(const QString &widgetId);

    // Выделить корневой элемент "Window" (без эмиссии сигнала)
    void selectWindow();

signals:
    // widgetId пустой = клик по "Window"
    void widgetSelected(const QString &widgetId);

private:
    void addWidgetToTree(WidgetItem *item, QTreeWidgetItem *parentTreeItem);

    QTreeWidget *m_tree = nullptr;
    DesignScene *m_scene = nullptr;
};

} // namespace DeltaQ

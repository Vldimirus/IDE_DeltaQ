// Диалог привязки событий — выбор обработчика для события виджета
#pragma once

#include <QDialog>
#include <QString>

class QTabWidget;
class QTreeWidget;
class QLineEdit;
class QDialogButtonBox;

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;

class EventBindingDialog : public QDialog {
    Q_OBJECT

public:
    explicit EventBindingDialog(ModuleRegistry *registry, GraphStore *graphStore,
                                 QWidget *parent = nullptr);

    // Установить текущее событие
    void setEventName(const QString &name);
    void setCurrentHandler(const QString &handler);

    // Результат
    QString selectedHandler() const { return m_selectedHandler; }
    QString eventName() const { return m_eventName; }

private:
    void buildModuleFunctionTab();
    void buildGraphTriggerTab();
    void buildCustomFunctionTab();

    ModuleRegistry *m_registry;
    GraphStore *m_graphStore;

    QTabWidget *m_tabWidget = nullptr;
    QTreeWidget *m_moduleFuncTree = nullptr;
    QTreeWidget *m_graphTriggerTree = nullptr;
    QLineEdit *m_customFuncEdit = nullptr;

    QString m_eventName;
    QString m_selectedHandler;
};

} // namespace DeltaQ

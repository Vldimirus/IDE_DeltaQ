// Модуль 4: Обработчик библиотек — полный виджет
#pragma once

#include <QWidget>

class QSplitter;
class QToolBar;
class QTreeWidget;
class QTextEdit;

namespace DeltaQ {

class ModuleRegistry;
class GraphStore;

class LibProcessorWidget : public QWidget {
    Q_OBJECT

public:
    explicit LibProcessorWidget(ModuleRegistry *registry,
                                 QWidget *parent = nullptr);

    void setGraphStore(GraphStore *store) { m_graphStore = store; }

private slots:
    void onImportLibrary();
    void onModuleSelected();

private:
    void setupUI();
    void setupToolBar();
    void refreshLibraryTree();

    ModuleRegistry *m_registry;
    GraphStore *m_graphStore = nullptr;

    QToolBar *m_toolbar = nullptr;
    QSplitter *m_splitter = nullptr;
    QTreeWidget *m_libraryTree = nullptr;
    QTextEdit *m_moduleDetails = nullptr;
};

} // namespace DeltaQ

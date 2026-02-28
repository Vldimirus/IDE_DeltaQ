// Модуль 2: Визуальный блочный редактор — QGraphicsView + BlockScene + toolbar
#pragma once

#include <QWidget>

class QGraphicsView;
class QSplitter;
class QToolBar;

namespace DeltaQ {

class ModuleRegistry;
class CommandBus;
class GraphStore;
class BlockScene;
class ModulePalette;

class BlockEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit BlockEditorWidget(ModuleRegistry *registry, CommandBus *bus,
                               QWidget *parent = nullptr);

    // Загрузка графа по id
    void loadGraph(const QString &graphId, GraphStore *store);
    void saveGraph(GraphStore *store);

    // Доступ к сцене
    BlockScene *scene() const { return m_scene; }

    // Палитра (добавляется в 3.2)
    void setPalette(ModulePalette *palette);

    // Масштабирование
    void zoomIn();
    void zoomOut();
    void zoomFit();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupToolBar();
    void deleteSelected();

    ModuleRegistry *m_registry;
    CommandBus *m_commandBus;
    BlockScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QToolBar *m_toolbar = nullptr;
    QSplitter *m_splitter = nullptr;

    bool m_firstShow = true;
    qreal m_currentZoom = 1.0;
    static constexpr qreal MinZoom = 0.1;
    static constexpr qreal MaxZoom = 5.0;
    static constexpr qreal ZoomStep = 1.15;
};

} // namespace DeltaQ

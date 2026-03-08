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
class BreadcrumbBar;

class BlockEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit BlockEditorWidget(ModuleRegistry *registry, CommandBus *bus,
                               QWidget *parent = nullptr);

    // Загрузка графа по id
    void loadGraph(const QString &graphId, GraphStore *store);
    void saveGraph(GraphStore *store);

    // Установить GraphStore для навигации
    void setGraphStore(GraphStore *store) { m_graphStore = store; }

    // Доступ к сцене
    BlockScene *scene() const { return m_scene; }

    // Палитра (добавляется в 3.2)
    void setPalette(ModulePalette *palette);

    // Масштабирование
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void modulePreviewRequested(const QString &moduleId);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

    // Навигация по подмодулям
    void navigateInto(const QString &graphId, const QString &label);
    void navigateBack();
    void navigateTo(int level);

private slots:
    void onSubModuleRequested(const QStringList &selectedNodeIds);
    void onNodeDoubleClicked(const QString &nodeId);

private:
    void setupToolBar();
    void deleteSelected();

    ModuleRegistry *m_registry;
    CommandBus *m_commandBus;
    GraphStore *m_graphStore = nullptr;
    BlockScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QToolBar *m_toolbar = nullptr;
    QSplitter *m_splitter = nullptr;
    BreadcrumbBar *m_breadcrumb = nullptr;

    // Стек навигации по подмодулям
    struct NavLevel {
        QString graphId;
        QString label;
    };
    QVector<NavLevel> m_navStack;

    bool m_firstShow = true;
    bool m_spacePressed = false;
    bool m_middleDragging = false;
    bool m_rightPanCandidate = false;
    bool m_suppressNextContextMenu = false;
    QPoint m_lastPanPos;
    Qt::MouseButton m_panButton = Qt::NoButton;
    qreal m_currentZoom = 1.0;
    static constexpr qreal MinZoom = 0.1;
    static constexpr qreal MaxZoom = 5.0;
    static constexpr qreal ZoomStep = 1.15;
};

} // namespace DeltaQ

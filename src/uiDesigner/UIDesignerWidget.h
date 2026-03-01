// Модуль 3: Дизайнер UI — полный виджет с холстом, палитрой и свойствами
#pragma once

#include <QWidget>

class QGraphicsView;
class QSplitter;
class QToolBar;
class QComboBox;

namespace DeltaQ {

class ModuleRegistry;
class CommandBus;
class UILayoutStore;
class DesignScene;
class WidgetPalette;
class PropertyEditor;
class ObjectTreeWidget;
class WidgetItem;

class UIDesignerWidget : public QWidget {
    Q_OBJECT

public:
    explicit UIDesignerWidget(ModuleRegistry *registry, CommandBus *bus,
                               QWidget *parent = nullptr);

    // Загрузка/сохранение макета
    void loadLayout(const QString &layoutId, UILayoutStore *store);
    void saveLayout(UILayoutStore *store);

    // Layout store и путь к файлу
    void setLayoutStore(UILayoutStore *store) { m_layoutStore = store; }
    void setFilePath(const QString &path) { m_currentFilePath = path; }
    QString filePath() const { return m_currentFilePath; }
    UILayoutStore *layoutStore() const { return m_layoutStore; }

    // Текущий UILayout (из store или создать Default)
    struct UILayout *currentLayout();

    // Доступ к сцене
    DesignScene *scene() const { return m_scene; }

    // Масштабирование
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void generateCodeRequested();
    void previewRequested();
    void openEventHandler(const QString &widgetId, const QString &widgetName, const QString &eventName);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleWidgetDropped(const QString &widgetType, const QPointF &scenePos, const QString &parentId);
    void handleWidgetMoved(const QString &widgetId, const QPointF &oldPos, const QPointF &newPos);
    void handleWidgetResized(const QString &widgetId, const QRectF &oldRect, const QRectF &newRect);
    void handleDeleteSelected();

private:
    void setupToolBar();
    void centerOnWindow();

    ModuleRegistry *m_registry;
    CommandBus *m_commandBus;
    UILayoutStore *m_layoutStore = nullptr;
    QString m_currentFilePath;

    DesignScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    WidgetPalette *m_widgetPalette = nullptr;
    PropertyEditor *m_propertyEditor = nullptr;
    ObjectTreeWidget *m_objectTree = nullptr;

    QToolBar *m_toolbar = nullptr;
    QSplitter *m_splitter = nullptr;
    QComboBox *m_layoutSelector = nullptr;

    QString m_currentLayoutId;

    qreal m_currentZoom = 1.0;
    bool m_firstShow = true;
    static constexpr qreal MinZoom = 0.1;
    static constexpr qreal MaxZoom = 5.0;
    static constexpr qreal ZoomStep = 1.15;
};

} // namespace DeltaQ

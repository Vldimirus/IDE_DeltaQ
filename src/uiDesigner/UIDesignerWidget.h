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
class WidgetItem;

class UIDesignerWidget : public QWidget {
    Q_OBJECT

public:
    explicit UIDesignerWidget(ModuleRegistry *registry, CommandBus *bus,
                               QWidget *parent = nullptr);

    // Загрузка/сохранение макета
    void loadLayout(const QString &layoutId, UILayoutStore *store);
    void saveLayout(UILayoutStore *store);

    // Доступ к сцене
    DesignScene *scene() const { return m_scene; }

    // Масштабирование
    void zoomIn();
    void zoomOut();
    void zoomFit();

signals:
    void generateCodeRequested();
    void previewRequested();

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupToolBar();

    ModuleRegistry *m_registry;
    CommandBus *m_commandBus;

    DesignScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    WidgetPalette *m_widgetPalette = nullptr;
    PropertyEditor *m_propertyEditor = nullptr;

    QToolBar *m_toolbar = nullptr;
    QSplitter *m_splitter = nullptr;
    QComboBox *m_layoutSelector = nullptr;

    QString m_currentLayoutId;

    qreal m_currentZoom = 1.0;
    static constexpr qreal MinZoom = 0.1;
    static constexpr qreal MaxZoom = 5.0;
    static constexpr qreal ZoomStep = 1.15;
};

} // namespace DeltaQ

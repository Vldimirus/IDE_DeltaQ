// Модуль 3: Дизайнер UI — реализация
#include "UIDesignerWidget.h"
#include "DesignScene.h"
#include "WidgetPalette.h"
#include "PropertyEditor.h"
#include "ObjectTreeWidget.h"
#include "WidgetItem.h"
#include "UICommands.h"

#include "../core/CommandBus.h"
#include "../core/ModuleRegistry.h"
#include "../core/UILayoutStore.h"

#include <QGraphicsView>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QWheelEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QAction>

namespace DeltaQ {

UIDesignerWidget::UIDesignerWidget(ModuleRegistry *registry, CommandBus *bus,
                                     QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
    , m_commandBus(bus)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    setupToolBar();
    mainLayout->addWidget(m_toolbar);

    // Сцена и вид
    m_scene = new DesignScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setAcceptDrops(true);
    m_view->viewport()->setAcceptDrops(true);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // Палитра виджетов
    m_widgetPalette = new WidgetPalette(this);
    m_widgetPalette->setMinimumWidth(180);
    m_widgetPalette->setMaximumWidth(250);

    // Редактор свойств
    m_propertyEditor = new PropertyEditor(this);
    m_propertyEditor->setMinimumWidth(200);
    m_propertyEditor->setMaximumWidth(300);

    // Дерево объектов
    m_objectTree = new ObjectTreeWidget(this);
    m_objectTree->setMinimumWidth(200);
    m_objectTree->setMaximumWidth(300);

    // Правая панель: дерево объектов (сверху) + свойства (снизу)
    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(m_objectTree);
    rightSplitter->addWidget(m_propertyEditor);
    rightSplitter->setSizes({200, 300});
    rightSplitter->setStretchFactor(0, 0);
    rightSplitter->setStretchFactor(1, 1);

    // Splitter: палитра | холст | правая панель
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_widgetPalette);
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(rightSplitter);
    m_splitter->setSizes({200, 600, 250});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    mainLayout->addWidget(m_splitter);

    // Подключение сигналов сцены — виджет сам обрабатывает drop/move/resize
    connect(m_scene, &DesignScene::widgetDropped,
            this, &UIDesignerWidget::handleWidgetDropped);
    connect(m_scene, &DesignScene::widgetMoved,
            this, &UIDesignerWidget::handleWidgetMoved);
    connect(m_scene, &DesignScene::widgetResized,
            this, &UIDesignerWidget::handleWidgetResized);

    // Выделение виджета → обновить PropertyEditor и Layout Selector
    connect(m_scene, &DesignScene::widgetSelected, this, [this](const QString &widgetId) {
        auto *item = m_scene->widgetItem(widgetId);
        if (item) {
            m_propertyEditor->setWidget(item);
            // Обновить layout selector по текущему виджету
            m_layoutSelector->blockSignals(true);
            int idx = m_layoutSelector->findText(item->layoutType());
            m_layoutSelector->setCurrentIndex(idx >= 0 ? idx : 0);
            m_layoutSelector->blockSignals(false);
        }
        m_objectTree->selectWidget(widgetId);
    });

    // Клик по пустому месту внутри окна → показать свойства окна
    connect(m_scene, &DesignScene::windowSelected, this, [this]() {
        m_propertyEditor->setWindowProperties(m_scene);
        m_objectTree->selectWindow();
    });

    // Клик по пустому месту вне окна → снять выделение
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        auto sel = m_scene->selectedItems();
        if (sel.isEmpty()) {
            // Не очищать, если показываются свойства окна
        }
        // Обновить дерево объектов
        m_objectTree->rebuild(m_scene);
    });

    // Дерево объектов: клик по элементу → выделение на сцене
    connect(m_objectTree, &ObjectTreeWidget::widgetSelected, this, [this](const QString &widgetId) {
        if (widgetId.isEmpty()) {
            // Клик по "Window" в дереве → показать свойства окна
            m_scene->clearSelection();
            m_propertyEditor->setWindowProperties(m_scene);
            return;
        }
        // Снять текущее выделение
        m_scene->clearSelection();
        auto *item = m_scene->widgetItem(widgetId);
        if (item) {
            item->setSelected(true);
            m_view->centerOn(item);
            m_propertyEditor->setWidget(item);
        }
    });

    setFocusPolicy(Qt::StrongFocus);
}

void UIDesignerWidget::setupToolBar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));

    m_toolbar->addAction(tr("Zoom +"), this, &UIDesignerWidget::zoomIn);
    m_toolbar->addAction(tr("Zoom -"), this, &UIDesignerWidget::zoomOut);
    m_toolbar->addAction(tr("Fit"), this, &UIDesignerWidget::zoomFit);
    m_toolbar->addSeparator();

    // Grid toggle
    auto *gridAction = m_toolbar->addAction(tr("Grid"));
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    connect(gridAction, &QAction::toggled, this, [this](bool on) {
        m_scene->setGridVisible(on);
    });

    m_toolbar->addSeparator();

    // Layout selector
    m_toolbar->addWidget(new QLabel(tr("Layout:")));
    m_layoutSelector = new QComboBox(this);
    m_layoutSelector->addItems({"None", "HBox", "VBox", "Grid", "Flow"});
    m_toolbar->addWidget(m_layoutSelector);

    // Подключение Layout Selector к ChangeLayoutCommand
    connect(m_layoutSelector, &QComboBox::currentTextChanged, this, [this](const QString &newLayout) {
        auto sel = m_scene->selectedItems();
        if (sel.isEmpty()) return;
        auto *item = qobject_cast<WidgetItem *>(
            dynamic_cast<QGraphicsObject *>(sel.first()));
        if (!item) return;

        UILayout *layout = currentLayout();
        if (!layout) return;

        QString oldLayout = item->layoutType();
        if (oldLayout == newLayout) return;

        m_commandBus->execute(std::make_unique<ChangeLayoutCommand>(
            m_scene, layout, item->widgetId(), oldLayout, newLayout));
    });

    m_toolbar->addSeparator();

    // Delete
    auto *deleteAction = m_toolbar->addAction(tr("Delete"));
    connect(deleteAction, &QAction::triggered, this, &UIDesignerWidget::handleDeleteSelected);

    m_toolbar->addSeparator();

    // Preview и Generate Code
    m_toolbar->addAction(tr("Preview"), this, [this]() {
        emit previewRequested();
    });
    m_toolbar->addAction(tr("Generate Code"), this, [this]() {
        emit generateCodeRequested();
    });
}

// --- Обработчики drag/move/resize ---

UILayout *UIDesignerWidget::currentLayout()
{
    if (!m_layoutStore) return nullptr;

    // Ищем по текущему layoutId
    if (!m_currentLayoutId.isEmpty()) {
        UILayout *layout = m_layoutStore->findLayout(m_currentLayoutId);
        if (layout) return layout;
    }

    // Ищем первый доступный
    auto layouts = m_layoutStore->allLayouts();
    if (!layouts.isEmpty()) {
        return m_layoutStore->findLayout(layouts.first()->id);
    }

    // Создаём Default
    UILayout newLayout = UILayout::create("Default");
    m_layoutStore->registerLayout(newLayout);
    m_currentLayoutId = newLayout.id;
    return m_layoutStore->findLayout(newLayout.id);
}

void UIDesignerWidget::handleWidgetDropped(const QString &widgetType, const QPointF &scenePos)
{
    UILayout *layout = currentLayout();
    if (!layout) return;

    UIWidget w = UIWidget::create(widgetType, widgetType + "_" +
        QString::number(m_scene->widgetItems().size() + 1));
    w.geometry = QRectF(scenePos.x(), scenePos.y(), 120, 40);

    if (widgetType == "Panel" || widgetType == "GroupBox")
        w.geometry.setSize(QSizeF(200, 150));
    else if (widgetType == "TextField" || widgetType == "ComboBox")
        w.geometry.setSize(QSizeF(150, 30));
    else if (widgetType == "Slider")
        w.geometry.setSize(QSizeF(200, 30));
    else if (widgetType == "ProgressBar")
        w.geometry.setSize(QSizeF(200, 24));
    else if (widgetType == "Image")
        w.geometry.setSize(QSizeF(100, 100));

    w.properties["text"] = w.name;

    m_commandBus->execute(std::make_unique<AddWidgetCommand>(m_scene, layout, w));

    // Обновить дерево объектов
    m_objectTree->rebuild(m_scene);
}

void UIDesignerWidget::handleWidgetMoved(const QString &widgetId,
                                          const QPointF &oldPos, const QPointF &newPos)
{
    UILayout *layout = currentLayout();
    if (!layout) return;

    m_commandBus->execute(std::make_unique<MoveWidgetCommand>(
        m_scene, layout, widgetId, oldPos, newPos));
}

void UIDesignerWidget::handleWidgetResized(const QString &widgetId,
                                            const QRectF &oldRect, const QRectF &newRect)
{
    UILayout *layout = currentLayout();
    if (!layout) return;

    m_commandBus->execute(std::make_unique<ResizeWidgetCommand>(
        m_scene, layout, widgetId, oldRect, newRect));
}

void UIDesignerWidget::handleDeleteSelected()
{
    auto sel = m_scene->selectedItems();
    if (sel.isEmpty()) return;

    UILayout *layout = currentLayout();
    if (!layout) return;

    if (sel.size() == 1) {
        auto *item = qobject_cast<WidgetItem *>(
            dynamic_cast<QGraphicsObject *>(sel.first()));
        if (item) {
            m_commandBus->execute(std::make_unique<RemoveWidgetCommand>(
                m_scene, layout, item->widgetId()));
        }
    } else {
        // Макрокоманда для удаления нескольких виджетов
        m_commandBus->beginMacro(tr("Delete %1 widgets").arg(sel.size()));
        for (auto *graphicsItem : sel) {
            auto *item = qobject_cast<WidgetItem *>(
                dynamic_cast<QGraphicsObject *>(graphicsItem));
            if (item) {
                m_commandBus->execute(std::make_unique<RemoveWidgetCommand>(
                    m_scene, layout, item->widgetId()));
            }
        }
        m_commandBus->endMacro();
    }

    m_propertyEditor->clearWidget();
    m_objectTree->rebuild(m_scene);
}

// --- Клавиатура ---

void UIDesignerWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        handleDeleteSelected();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

// --- Загрузка/сохранение ---

void UIDesignerWidget::loadLayout(const QString &layoutId, UILayoutStore *store)
{
    if (!store) return;
    auto *layout = store->findLayout(layoutId);
    if (!layout) return;

    m_layoutStore = store;
    m_currentLayoutId = layoutId;
    m_scene->loadFromLayout(*layout);
    m_objectTree->rebuild(m_scene);
}

void UIDesignerWidget::saveLayout(UILayoutStore *store)
{
    if (!store || m_currentLayoutId.isEmpty()) return;
    auto *existing = store->findLayout(m_currentLayoutId);
    if (!existing) return;

    UILayout layout = m_scene->toLayout(existing->name);
    layout.id = m_currentLayoutId;
    layout.version = existing->version;
    layout.resources = existing->resources;
    layout.metadata = existing->metadata;

    *existing = layout;
}

// --- Масштабирование ---

void UIDesignerWidget::zoomIn()
{
    if (m_currentZoom >= MaxZoom) return;
    m_currentZoom *= ZoomStep;
    m_view->resetTransform();
    m_view->scale(m_currentZoom, m_currentZoom);
}

void UIDesignerWidget::zoomOut()
{
    if (m_currentZoom <= MinZoom) return;
    m_currentZoom /= ZoomStep;
    m_view->resetTransform();
    m_view->scale(m_currentZoom, m_currentZoom);
}

void UIDesignerWidget::zoomFit()
{
    m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    m_currentZoom = m_view->transform().m11();
}

void UIDesignerWidget::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0)
            zoomIn();
        else
            zoomOut();
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void UIDesignerWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_firstShow) {
        m_firstShow = false;
        centerOnWindow();
    }
}

void UIDesignerWidget::centerOnWindow()
{
    QRectF windowRect = m_scene->windowRect();
    // Добавляем отступ вокруг рамки окна для комфортного обзора
    QRectF viewRect = windowRect.adjusted(-40, -40, 40, 40);
    m_view->fitInView(viewRect, Qt::KeepAspectRatio);
    m_currentZoom = m_view->transform().m11();
}

} // namespace DeltaQ

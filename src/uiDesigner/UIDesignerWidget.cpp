// Модуль 3: Дизайнер UI — реализация
#include "UIDesignerWidget.h"
#include "DesignScene.h"
#include "WidgetPalette.h"
#include "PropertyEditor.h"
#include "WidgetItem.h"

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
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setAcceptDrops(true);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

    // Палитра виджетов
    m_widgetPalette = new WidgetPalette(this);
    m_widgetPalette->setMinimumWidth(180);
    m_widgetPalette->setMaximumWidth(250);

    // Редактор свойств
    m_propertyEditor = new PropertyEditor(this);
    m_propertyEditor->setMinimumWidth(200);
    m_propertyEditor->setMaximumWidth(300);

    // Splitter: палитра | холст | свойства
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_widgetPalette);
    m_splitter->addWidget(m_view);
    m_splitter->addWidget(m_propertyEditor);
    m_splitter->setSizes({200, 600, 250});
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    mainLayout->addWidget(m_splitter);

    // Подключения
    connect(m_scene, &DesignScene::widgetSelected, this, [this](const QString &widgetId) {
        auto *item = m_scene->widgetItem(widgetId);
        if (item)
            m_propertyEditor->setWidget(item);
    });

    // Клик по пустому месту → снять выделение
    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this]() {
        auto sel = m_scene->selectedItems();
        if (sel.isEmpty())
            m_propertyEditor->clearWidget();
    });
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

    m_toolbar->addSeparator();

    // Preview и Generate Code
    m_toolbar->addAction(tr("Preview"), this, [this]() {
        emit previewRequested();
    });
    m_toolbar->addAction(tr("Generate Code"), this, [this]() {
        emit generateCodeRequested();
    });
}

// --- Загрузка/сохранение ---

void UIDesignerWidget::loadLayout(const QString &layoutId, UILayoutStore *store)
{
    if (!store) return;
    auto *layout = store->findLayout(layoutId);
    if (!layout) return;

    m_currentLayoutId = layoutId;
    m_scene->loadFromLayout(*layout);
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

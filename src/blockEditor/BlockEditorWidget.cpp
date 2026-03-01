// Модуль 2: Визуальный блочный редактор — реализация
#include "BlockEditorWidget.h"
#include "BlockScene.h"
#include "ModulePalette.h"

#include "NodeItem.h"
#include "ConnectionItem.h"
#include "PortItem.h"
#include "GraphCommands.h"

#include "../core/GraphStore.h"
#include "../core/CommandBus.h"

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QScrollBar>

namespace DeltaQ {

BlockEditorWidget::BlockEditorWidget(ModuleRegistry *registry, CommandBus *bus,
                                     QWidget *parent)
    : QWidget(parent)
    , m_registry(registry)
    , m_commandBus(bus)
{
    m_scene = new BlockScene(registry, bus, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setAcceptDrops(true);
    m_view->viewport()->installEventFilter(this);

    setupToolBar();

    // Основной layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_toolbar);

    // Splitter: палитра слева (будет добавлена в 3.2), холст справа
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_view);
    m_splitter->setStretchFactor(0, 1); // view растягивается

    mainLayout->addWidget(m_splitter);
}

void BlockEditorWidget::setupToolBar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setMovable(false);

    auto *zoomInAct = m_toolbar->addAction("+");
    zoomInAct->setToolTip(tr("Zoom In (Ctrl++)"));
    connect(zoomInAct, &QAction::triggered, this, &BlockEditorWidget::zoomIn);

    auto *zoomOutAct = m_toolbar->addAction("-");
    zoomOutAct->setToolTip(tr("Zoom Out (Ctrl+-)"));
    connect(zoomOutAct, &QAction::triggered, this, &BlockEditorWidget::zoomOut);

    auto *fitAct = m_toolbar->addAction("Fit");
    fitAct->setToolTip(tr("Fit In View (Ctrl+0)"));
    connect(fitAct, &QAction::triggered, this, &BlockEditorWidget::zoomFit);
}

void BlockEditorWidget::loadGraph(const QString &graphId, GraphStore *store)
{
    if (!store) return;

    Graph *g = store->findGraph(graphId);
    if (!g) return;

    m_scene->loadFromGraph(*g);
}

void BlockEditorWidget::saveGraph(GraphStore *store)
{
    if (!store || m_scene->currentGraphId().isEmpty()) return;

    Graph *existing = store->findGraph(m_scene->currentGraphId());
    if (!existing) return;

    Graph updated = m_scene->toGraph(existing->id, existing->name);
    *existing = updated;
}

void BlockEditorWidget::setPalette(ModulePalette *palette)
{
    if (!palette) return;

    // Вставляем палитру слева от view
    m_splitter->insertWidget(0, palette);
    m_splitter->setSizes({200, 600}); // палитра 200px, view — остальное
}

void BlockEditorWidget::zoomIn()
{
    if (m_currentZoom * ZoomStep <= MaxZoom) {
        m_view->scale(ZoomStep, ZoomStep);
        m_currentZoom *= ZoomStep;
    }
}

void BlockEditorWidget::zoomOut()
{
    if (m_currentZoom / ZoomStep >= MinZoom) {
        m_view->scale(1.0 / ZoomStep, 1.0 / ZoomStep);
        m_currentZoom /= ZoomStep;
    }
}

void BlockEditorWidget::zoomFit()
{
    QRectF rect = m_scene->itemsBoundingRect().adjusted(-50, -50, 50, 50);
    m_view->fitInView(rect, Qt::KeepAspectRatio);
    m_currentZoom = m_view->transform().m11();
    // Ограничиваем зум — узлы не должны быть огромными
    if (m_currentZoom > 1.5) {
        m_view->resetTransform();
        m_currentZoom = 1.0;
        m_view->centerOn(rect.center());
    }
}

bool BlockEditorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_view->viewport())
        return QWidget::eventFilter(obj, event);

    // Панорамирование средней кнопкой мыши
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::MiddleButton) {
            m_middleDragging = true;
            m_lastPanPos = me->pos();
            m_view->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        // Space + ЛКМ → панорамирование
        if (me->button() == Qt::LeftButton && m_spacePressed) {
            m_middleDragging = true;
            m_lastPanPos = me->pos();
            m_view->setCursor(Qt::ClosedHandCursor);
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (m_middleDragging) {
            QPoint delta = me->pos() - m_lastPanPos;
            m_lastPanPos = me->pos();
            m_view->horizontalScrollBar()->setValue(
                m_view->horizontalScrollBar()->value() - delta.x());
            m_view->verticalScrollBar()->setValue(
                m_view->verticalScrollBar()->value() - delta.y());
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::MiddleButton ||
            (me->button() == Qt::LeftButton && m_middleDragging)) {
            m_middleDragging = false;
            m_view->setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void BlockEditorWidget::wheelEvent(QWheelEvent *event)
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

void BlockEditorWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
            zoomIn();
            return;
        }
        if (event->key() == Qt::Key_Minus) {
            zoomOut();
            return;
        }
        if (event->key() == Qt::Key_0) {
            zoomFit();
            return;
        }
        if (event->key() == Qt::Key_A) {
            // Выделить всё
            for (auto *item : m_scene->items())
                item->setSelected(true);
            return;
        }
    }

    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = true;
        m_view->setCursor(Qt::OpenHandCursor);
        return;
    }

    if (event->key() == Qt::Key_Delete) {
        deleteSelected();
        return;
    }

    QWidget::keyPressEvent(event);
}

void BlockEditorWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePressed = false;
        if (!m_middleDragging)
            m_view->setCursor(Qt::ArrowCursor);
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void BlockEditorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_firstShow && !m_scene->items().isEmpty()) {
        m_firstShow = false;
        zoomFit();
        // Ограничиваем максимальный зум, чтобы узлы не были огромными
        if (m_currentZoom > 1.5) {
            m_view->resetTransform();
            m_view->scale(1.0, 1.0);
            m_currentZoom = 1.0;
            m_view->centerOn(m_scene->itemsBoundingRect().center());
        }
    }
}

void BlockEditorWidget::deleteSelected()
{
    auto selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;

    // Собираем данные ДО удаления (после delete указатели невалидны)
    struct ConnInfo {
        QString fromNode, fromPort, toNode, toPort;
    };
    QList<ConnInfo> connsToRemove;
    QStringList nodesToRemove;

    for (auto *item : selected) {
        auto *conn = dynamic_cast<ConnectionItem *>(item);
        if (conn && conn->sourcePort() && conn->destPort()) {
            connsToRemove.append({
                conn->sourcePort()->parentNode()->nodeId(),
                conn->sourcePort()->portName(),
                conn->destPort()->parentNode()->nodeId(),
                conn->destPort()->portName()
            });
            continue;
        }
        auto *node = dynamic_cast<NodeItem *>(item);
        if (node)
            nodesToRemove.append(node->nodeId());
    }

    // Удаляем соединения по сохранённым данным
    for (const auto &c : connsToRemove)
        m_scene->removeConnectionItem(c.fromNode, c.fromPort, c.toNode, c.toPort);

    // Удаляем узлы по сохранённым ID
    for (const auto &id : nodesToRemove)
        m_scene->removeNodeItem(id);
}

} // namespace DeltaQ

// Модуль 2: Визуальный блочный редактор — реализация
#include "BlockEditorWidget.h"
#include "BlockScene.h"
#include "ModulePalette.h"
#include "BreadcrumbBar.h"
#include "SubModuleFactory.h"

#include "NodeItem.h"
#include "ConnectionItem.h"
#include "PortItem.h"
#include "GraphCommands.h"

#include "../core/GraphStore.h"
#include "../core/CommandBus.h"
#include "../core/ModuleRegistry.h"

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QApplication>
#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QInputDialog>
#include <QMessageBox>

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
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setRubberBandSelectionMode(Qt::IntersectsItemShape);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setAcceptDrops(true);
    m_view->viewport()->installEventFilter(this);

    setupToolBar();

    // Хлебные крошки
    m_breadcrumb = new BreadcrumbBar(this);
    connect(m_breadcrumb, &BreadcrumbBar::backClicked, this, &BlockEditorWidget::navigateBack);
    connect(m_breadcrumb, &BreadcrumbBar::levelClicked, this, &BlockEditorWidget::navigateTo);

    // Сигналы подмодулей
    connect(m_scene, &BlockScene::subModuleRequested,
            this, &BlockEditorWidget::onSubModuleRequested);
    connect(m_scene, &BlockScene::nodeDoubleClicked,
            this, &BlockEditorWidget::onNodeDoubleClicked);

    // Основной layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_breadcrumb);
    m_breadcrumb->setVisible(false); // Показывается только при навигации
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

    if (event->type() == QEvent::ContextMenu && m_suppressNextContextMenu) {
        m_suppressNextContextMenu = false;
        return true;
    }

    if (event->type() == QEvent::Wheel) {
        auto *we = static_cast<QWheelEvent *>(event);
        if (we->angleDelta().y() > 0)
            zoomIn();
        else if (we->angleDelta().y() < 0)
            zoomOut();
        we->accept();
        return true;
    }

    // Панорамирование средней кнопкой мыши
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::MiddleButton) {
            m_middleDragging = true;
            m_panButton = Qt::MiddleButton;
            m_lastPanPos = me->pos();
            m_view->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        if (me->button() == Qt::RightButton) {
            m_rightPanCandidate = true;
            m_lastPanPos = me->pos();
            return false;
        }
        // Space + ЛКМ → панорамирование
        if (me->button() == Qt::LeftButton && m_spacePressed) {
            m_middleDragging = true;
            m_panButton = Qt::LeftButton;
            m_lastPanPos = me->pos();
            m_view->setCursor(Qt::ClosedHandCursor);
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (m_rightPanCandidate && (me->buttons() & Qt::RightButton) &&
            (me->pos() - m_lastPanPos).manhattanLength() >= QApplication::startDragDistance()) {
            m_middleDragging = true;
            m_panButton = Qt::RightButton;
            m_rightPanCandidate = false;
            m_suppressNextContextMenu = true;
            m_view->setCursor(Qt::ClosedHandCursor);
        }

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
        if (me->button() == Qt::RightButton && m_rightPanCandidate) {
            m_rightPanCandidate = false;
            return false;
        }
        if (m_middleDragging && me->button() == m_panButton) {
            m_middleDragging = false;
            m_panButton = Qt::NoButton;
            m_view->setCursor(m_spacePressed ? Qt::OpenHandCursor : Qt::ArrowCursor);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void BlockEditorWidget::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        zoomIn();
    else if (event->angleDelta().y() < 0)
        zoomOut();
    event->accept();
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

// --- Подмодули ---

void BlockEditorWidget::onSubModuleRequested(const QStringList &selectedNodeIds)
{
    if (!m_graphStore || selectedNodeIds.size() < 2) return;

    // Запрашиваем имя у пользователя
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Создать подмодуль"),
        tr("Имя подмодуля:"), QLineEdit::Normal, "SubModule", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed().replace(' ', '_');

    // Получаем текущий граф
    QString graphId = m_scene->currentGraphId();
    Graph *currentGraph = m_graphStore->findGraph(graphId);
    if (!currentGraph) return;

    // Создаём подмодуль через фабрику
    auto result = SubModuleFactory::createFromSelection(
        *currentGraph, selectedNodeIds, name, m_registry);

    // Регистрируем модуль и граф
    m_registry->registerModule(result.module);
    m_graphStore->registerGraph(result.innerGraph);

    // Вычисляем среднюю позицию выделенных узлов
    QPointF avgPos;
    int count = 0;
    for (const auto &nodeId : selectedNodeIds) {
        const GraphNode *node = currentGraph->findNode(nodeId);
        if (node) {
            avgPos += node->position;
            count++;
        }
    }
    if (count > 0) avgPos /= count;

    // Удаляем выделенные узлы из текущего графа
    for (const auto &nodeId : selectedNodeIds)
        currentGraph->removeNode(nodeId);

    // Добавляем узел подмодуля в текущий граф
    GraphNode subNode = GraphNode::create(result.module.id, avgPos);
    currentGraph->addNode(subNode);

    // Перезагружаем граф на сцене
    m_scene->loadFromGraph(*currentGraph);

    // Сохраняем .dqmod в dqmods/ если проект открыт
    if (m_graphStore) {
        m_registry->saveModuleFile(result.module,
            m_graphStore->findGraph(graphId) ? "" : "");
    }
}

void BlockEditorWidget::onNodeDoubleClicked(const QString &nodeId)
{
    if (!m_graphStore) return;

    // Найти граф с этим узлом
    QString graphId = m_scene->currentGraphId();
    Graph *currentGraph = m_graphStore->findGraph(graphId);
    if (!currentGraph) return;

    const GraphNode *node = currentGraph->findNode(nodeId);
    if (!node) return;

    // Проверяем, есть ли у модуля graphId (является ли он композитным)
    const Module *mod = m_registry->findModule(node->moduleId);
    if (!mod) return;

    if (mod->graphId.isEmpty()) {
        emit modulePreviewRequested(mod->id);
        return;
    }

    // Навигация внутрь подмодуля
    navigateInto(mod->graphId, mod->name);
}

// --- Навигация по подмодулям ---

void BlockEditorWidget::navigateInto(const QString &graphId, const QString &label)
{
    if (!m_graphStore) return;

    Graph *innerGraph = m_graphStore->findGraph(graphId);
    if (!innerGraph) return;

    // Сохраняем текущий уровень в стек
    if (m_navStack.isEmpty()) {
        // Первый переход — сохраняем корневой граф
        m_navStack.append({m_scene->currentGraphId(), tr("Главный граф")});
    }
    m_navStack.append({graphId, label});

    // Загружаем внутренний граф
    m_scene->loadFromGraph(*innerGraph);

    // Обновляем хлебные крошки
    QStringList labels;
    for (const auto &level : m_navStack)
        labels.append(level.label);
    m_breadcrumb->setPath(labels);
    m_breadcrumb->setVisible(true);
}

void BlockEditorWidget::navigateBack()
{
    if (m_navStack.size() <= 1) return;

    m_navStack.removeLast();
    const NavLevel &level = m_navStack.last();

    Graph *graph = m_graphStore->findGraph(level.graphId);
    if (graph)
        m_scene->loadFromGraph(*graph);

    if (m_navStack.size() <= 1) {
        // Вернулись на корневой уровень
        m_navStack.clear();
        m_breadcrumb->setVisible(false);
    } else {
        QStringList labels;
        for (const auto &l : m_navStack)
            labels.append(l.label);
        m_breadcrumb->setPath(labels);
    }
}

void BlockEditorWidget::navigateTo(int level)
{
    if (level < 0 || level >= m_navStack.size()) return;

    // Обрезаем стек до нужного уровня
    while (m_navStack.size() > level + 1)
        m_navStack.removeLast();

    const NavLevel &nav = m_navStack.last();
    Graph *graph = m_graphStore->findGraph(nav.graphId);
    if (graph)
        m_scene->loadFromGraph(*graph);

    if (m_navStack.size() <= 1) {
        m_navStack.clear();
        m_breadcrumb->setVisible(false);
    } else {
        QStringList labels;
        for (const auto &l : m_navStack)
            labels.append(l.label);
        m_breadcrumb->setPath(labels);
    }
}

} // namespace DeltaQ

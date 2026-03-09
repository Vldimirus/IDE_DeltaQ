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
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

namespace DeltaQ {

namespace {

// Приводит введённое пользователем имя файла к безопасному базовому имени без расширения.
QString sanitizeGeneratedFileBaseName(const QString &value)
{
    QString result = value.trimmed().toLower();
    for (QChar &ch : result) {
        if (!ch.isLetterOrNumber())
            ch = '_';
    }
    while (result.contains("__"))
        result.replace("__", "_");
    while (result.startsWith('_'))
        result.remove(0, 1);
    while (result.endsWith('_'))
        result.chop(1);
    return result;
}

// Старый fallback для подмодулей без явно заданного generated file base.
QString legacyGeneratedFileBaseName(const Module &module)
{
    QString base = sanitizeGeneratedFileBaseName(module.name);
    if (base.isEmpty())
        base = "submodule";
    if (!module.id.isEmpty())
        return base + "_" + module.id.left(8);
    return base;
}

// Проверяет, занято ли базовое имя generated-файла другим составным модулем.
bool generatedFileBaseNameExists(const ModuleRegistry *registry, const QString &fileBaseName)
{
    if (!registry)
        return false;

    const QString normalized = sanitizeGeneratedFileBaseName(fileBaseName);
    if (normalized.isEmpty())
        return false;

    for (const auto *module : registry->allModules()) {
        if (!module || !module->isComposite())
            continue;

        const QString currentBase = module->generatedFileBaseName.trimmed().isEmpty()
            ? legacyGeneratedFileBaseName(*module)
            : sanitizeGeneratedFileBaseName(module->generatedFileBaseName);
        if (currentBase == normalized)
            return true;
    }

    return false;
}

// Подбирает следующее свободное имя по шаблону submodule_01, submodule_02, ...
QString nextDefaultGeneratedFileBaseName(const ModuleRegistry *registry)
{
    int index = 1;
    while (true) {
        const QString candidate = QString("submodule_%1").arg(index, 2, 10, QChar('0'));
        if (!generatedFileBaseNameExists(registry, candidate))
            return candidate;
        index++;
    }
}

} // namespace

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
    const QRectF rect = fitTargetRect();
    if (!rect.isValid() || rect.isEmpty())
        return;
    applyAutoFitTransform(rect);
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
    }
}

QRectF BlockEditorWidget::fitTargetRect() const
{
    const QRectF itemsRect = m_scene->itemsBoundingRect();
    if (!itemsRect.isValid() || itemsRect.isEmpty())
        return {};

    QRectF targetRect = itemsRect.adjusted(-50, -50, 50, 50);

    const QSize viewportSize = m_view->viewport()->size();
    if (viewportSize.width() > 0 && targetRect.width() < viewportSize.width()) {
        const qreal grow = (viewportSize.width() - targetRect.width()) * 0.5;
        targetRect.adjust(-grow, 0.0, grow, 0.0);
    }
    if (viewportSize.height() > 0 && targetRect.height() < viewportSize.height()) {
        const qreal grow = (viewportSize.height() - targetRect.height()) * 0.5;
        targetRect.adjust(0.0, -grow, 0.0, grow);
    }

    return targetRect;
}

void BlockEditorWidget::applyAutoFitTransform(const QRectF &targetRect)
{
    m_view->fitInView(targetRect, Qt::KeepAspectRatio);
    m_currentZoom = m_view->transform().m11();

    // Fit не должен искусственно "раздувать" маленький граф. Авто-fit уменьшает, но не увеличивает
    // граф выше естественного масштаба 1:1.
    if (m_currentZoom > AutoFitMaxZoom) {
        m_view->resetTransform();
        m_currentZoom = AutoFitMaxZoom;
        m_view->centerOn(targetRect.center());
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

    // При создании подмодуля пользователь задаёт и имя модуля, и базовое имя generated .h/.c.
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Создать подмодуль"));

    auto *layout = new QFormLayout(&dialog);
    auto *nameEdit = new QLineEdit("SubModule", &dialog);
    auto *fileBaseEdit = new QLineEdit(nextDefaultGeneratedFileBaseName(m_registry), &dialog);
    fileBaseEdit->setPlaceholderText(tr("Например: submodule_01"));
    layout->addRow(tr("Имя подмодуля:"), nameEdit);
    layout->addRow(tr("Имя generated-файла:"), fileBaseEdit);

    auto *hint = new QLabel(
        tr("Будут созданы файлы <имя>.h и <имя>.c в каталоге generated/submodules."),
        &dialog);
    hint->setWordWrap(true);
    layout->addRow(hint);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString name = nameEdit->text().trimmed();
    if (name.isEmpty())
        return;
    name = name.trimmed().replace(' ', '_');

    QString fileBaseName = sanitizeGeneratedFileBaseName(fileBaseEdit->text());
    if (fileBaseName.isEmpty())
        fileBaseName = nextDefaultGeneratedFileBaseName(m_registry);

    if (generatedFileBaseNameExists(m_registry, fileBaseName)) {
        QMessageBox::warning(
            this,
            tr("Имя generated-файла занято"),
            tr("Базовое имя '%1' уже используется другим подмодулем. "
               "Укажите другое имя generated-файла.")
                .arg(fileBaseName));
        return;
    }

    // Получаем текущий граф
    QString graphId = m_scene->currentGraphId();
    Graph *currentGraph = m_graphStore->findGraph(graphId);
    if (!currentGraph) return;

    // Создаём подмодуль через фабрику
    auto result = SubModuleFactory::createFromSelection(
        *currentGraph, selectedNodeIds, name, m_registry);
    result.module.generatedFileBaseName = fileBaseName;

    // Регистрируем модуль и внутренний граф подмодуля
    m_registry->registerModule(result.module);
    m_graphStore->registerGraph(result.innerGraph);

    // Подменяем выделение на один узел подмодуля, сохраняя внешние exec/data связи.
    *currentGraph = result.updatedParentGraph;

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

// Тесты BlockEditorWidget — zoomFit и поведение auto-fit
#include <QtTest>
#include <QGraphicsView>
#include <deltaq/Graph.h>

#include "../../src/blockEditor/BlockEditorWidget.h"
#include "../../src/blockEditor/BlockScene.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/CommandBus.h"

using namespace DeltaQ;

class TestBlockEditorWidget : public QObject {
    Q_OBJECT

private:
    static void registerTestModule(ModuleRegistry &registry)
    {
        Module mod;
        mod.id = "add";
        mod.name = "Add";
        mod.category = "math";
        mod.language = "c";
        mod.version = "1.0";
        mod.origin = "user";
        mod.compileStatus = "passed";
        mod.testStatus = "passed";
        mod.inputs = {{"a", "int", "0"}, {"b", "int", "0"}};
        mod.outputs = {{"result", "int", ""}};
        mod.sourceCode = "int dq_add(int a, int b) {\n"
                         "    return a + b;\n"
                         "}";
        QVERIFY(registry.registerModule(mod));
    }

private slots:
    void zoomFitDoesNotOverscaleSingleNode()
    {
        ModuleRegistry registry;
        CommandBus bus;
        registerTestModule(registry);

        BlockEditorWidget widget(&registry, &bus);
        widget.resize(1100, 700);

        GraphNode node = GraphNode::create("add", QPointF(0, 0));
        widget.scene()->addNodeItem(node);

        widget.show();
        QCoreApplication::processEvents();

        widget.zoomFit();
        QCoreApplication::processEvents();

        auto *view = widget.findChild<QGraphicsView *>();
        QVERIFY(view != nullptr);
        QVERIFY2(view->transform().m11() <= 1.01,
                 qPrintable(QString("Unexpected zoom factor: %1").arg(view->transform().m11())));
    }

    void zoomFitStillZoomsOutForWideGraph()
    {
        ModuleRegistry registry;
        CommandBus bus;
        registerTestModule(registry);

        BlockEditorWidget widget(&registry, &bus);
        widget.resize(900, 600);

        GraphNode left = GraphNode::create("add", QPointF(0, 0));
        GraphNode right = GraphNode::create("add", QPointF(2600, 0));
        widget.scene()->addNodeItem(left);
        widget.scene()->addNodeItem(right);

        widget.show();
        QCoreApplication::processEvents();

        widget.zoomFit();
        QCoreApplication::processEvents();

        auto *view = widget.findChild<QGraphicsView *>();
        QVERIFY(view != nullptr);
        QVERIFY2(view->transform().m11() < 1.0,
                 qPrintable(QString("Expected zoom-out factor, got: %1").arg(view->transform().m11())));
    }
};

QTEST_MAIN(TestBlockEditorWidget)
#include "test_BlockEditorWidget.moc"

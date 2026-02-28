// Тесты DesignScene
#include <QTest>
#include <QApplication>
#include "../../src/uiDesigner/DesignScene.h"
#include "../../src/uiDesigner/WidgetItem.h"
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestDesignScene : public QObject {
    Q_OBJECT

private slots:
    void testAddWidget()
    {
        DesignScene scene;
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);

        auto *item = scene.addWidgetItem(w);
        QVERIFY(item != nullptr);
        QCOMPARE(item->widgetId(), w.id);
        QCOMPARE(scene.widgetItems().size(), 1);
    }

    void testRemoveWidget()
    {
        DesignScene scene;
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);

        scene.addWidgetItem(w);
        QCOMPARE(scene.widgetItems().size(), 1);

        scene.removeWidgetItem(w.id);
        QCOMPARE(scene.widgetItems().size(), 0);
    }

    void testFindWidget()
    {
        DesignScene scene;
        UIWidget w1 = UIWidget::create("Button", "btn1");
        w1.geometry = QRectF(10, 20, 100, 40);
        UIWidget w2 = UIWidget::create("Label", "lbl1");
        w2.geometry = QRectF(10, 80, 100, 30);

        scene.addWidgetItem(w1);
        scene.addWidgetItem(w2);

        auto *found = scene.widgetItem(w1.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->widgetType(), QString("Button"));

        auto *notFound = scene.widgetItem("nonexistent");
        QVERIFY(notFound == nullptr);
    }

    void testLoadFromLayoutAndToLayout()
    {
        DesignScene scene;
        UILayout layout = UILayout::create("TestLayout");

        UIWidget btn = UIWidget::create("Button", "btn1");
        btn.geometry = QRectF(10, 20, 100, 40);
        btn.properties["text"] = "Click";
        layout.window.children.append(btn);

        UIWidget lbl = UIWidget::create("Label", "lbl1");
        lbl.geometry = QRectF(10, 80, 100, 30);
        layout.window.children.append(lbl);

        scene.loadFromLayout(layout);
        QCOMPARE(scene.widgetItems().size(), 2);

        // Round-trip
        UILayout result = scene.toLayout("TestLayout");
        QCOMPARE(result.window.children.size(), 2);
    }

    void testClearScene()
    {
        DesignScene scene;
        UIWidget w = UIWidget::create("Button", "btn");
        w.geometry = QRectF(0, 0, 100, 40);

        scene.addWidgetItem(w);
        QCOMPARE(scene.widgetItems().size(), 1);

        scene.clearScene();
        QCOMPARE(scene.widgetItems().size(), 0);
    }

    void testGridVisibility()
    {
        DesignScene scene;
        QVERIFY(scene.isGridVisible());

        scene.setGridVisible(false);
        QVERIFY(!scene.isGridVisible());

        scene.setGridVisible(true);
        QVERIFY(scene.isGridVisible());
    }

    void testDuplicateAdd()
    {
        DesignScene scene;
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);

        auto *item1 = scene.addWidgetItem(w);
        auto *item2 = scene.addWidgetItem(w); // Повторное добавление
        QCOMPARE(item1, item2); // Должен вернуть тот же
        QCOMPARE(scene.widgetItems().size(), 1);
    }

    void testMultipleWidgets()
    {
        DesignScene scene;
        for (int i = 0; i < 10; ++i) {
            UIWidget w = UIWidget::create("Button", QString("btn_%1").arg(i));
            w.geometry = QRectF(i * 110, 0, 100, 40);
            scene.addWidgetItem(w);
        }
        QCOMPARE(scene.widgetItems().size(), 10);
    }
};

QTEST_MAIN(TestDesignScene)
#include "test_DesignScene.moc"

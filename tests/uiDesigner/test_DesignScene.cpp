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
        layout.window.geometry = QRectF(0, 0, 900, 620);
        layout.window.properties["title"] = "Text Pad";
        layout.window.properties["min_width"] = 420;
        layout.window.properties["min_height"] = 300;
        layout.window.properties["resizable"] = false;
        layout.window.properties["margin"] = 12;

        UIWidget btn = UIWidget::create("Button", "btn1");
        btn.geometry = QRectF(10, 20, 100, 40);
        btn.properties["text"] = "Click";
        layout.window.children.append(btn);

        UIWidget lbl = UIWidget::create("Label", "lbl1");
        lbl.geometry = QRectF(10, 80, 100, 30);
        layout.window.children.append(lbl);

        scene.loadFromLayout(layout);
        QCOMPARE(scene.widgetItems().size(), 2);
        QCOMPARE(scene.windowRect(), QRectF(0, 0, 900, 620));
        QCOMPARE(scene.windowTitle(), QString("Text Pad"));
        QCOMPARE(scene.windowMinimumSize(), QSizeF(420, 300));
        QCOMPARE(scene.windowResizable(), false);

        // Round-trip
        UILayout result = scene.toLayout("TestLayout");
        QCOMPARE(result.window.children.size(), 2);
        QCOMPARE(result.window.geometry, QRectF(0, 0, 900, 620));
        QCOMPARE(result.window.properties.value("title").toString(), QString("Text Pad"));
        QCOMPARE(result.window.properties.value("min_width").toInt(), 420);
        QCOMPARE(result.window.properties.value("min_height").toInt(), 300);
        QCOMPARE(result.window.properties.value("resizable").toBool(), false);
        QCOMPARE(result.window.properties.value("margin").toInt(), 12);
    }

    void testWindowMinimumSizeClampsGeometry()
    {
        DesignScene scene;
        scene.setWindowMinimumSize(QSizeF(480, 360));
        scene.setWindowRect(QRectF(0, 0, 320, 240));

        QCOMPARE(scene.windowRect(), QRectF(0, 0, 480, 360));
        QCOMPARE(scene.windowMinimumSize(), QSizeF(480, 360));
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

    void testContainerTypeUsesSharedContractVocabulary()
    {
        QVERIFY(DesignScene::isContainerType("Panel"));
        QVERIFY(DesignScene::isContainerType("panel"));
        QVERIFY(DesignScene::isContainerType("GroupBox"));
        QVERIFY(!DesignScene::isContainerType("Button"));
    }
};

QTEST_MAIN(TestDesignScene)
#include "test_DesignScene.moc"

// Тесты UICommands
#include <QTest>
#include <QApplication>
#include "../../src/uiDesigner/DesignScene.h"
#include "../../src/uiDesigner/WidgetItem.h"
#include "../../src/uiDesigner/UICommands.h"
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestUICommands : public QObject {
    Q_OBJECT

private:
    DesignScene *m_scene = nullptr;
    UILayout m_layout;

    void setup()
    {
        m_scene = new DesignScene;
        m_layout = UILayout::create("TestLayout");
    }

    void cleanup()
    {
        delete m_scene;
        m_scene = nullptr;
    }

private slots:
    void testAddWidgetCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);

        AddWidgetCommand cmd(m_scene, &m_layout, w);
        cmd.execute();
        QCOMPARE(m_scene->widgetItems().size(), 1);
        QCOMPARE(m_layout.window.children.size(), 1);

        cmd.undo();
        QCOMPARE(m_scene->widgetItems().size(), 0);
        QCOMPARE(m_layout.window.children.size(), 0);
        cleanup();
    }

    void testRemoveWidgetCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);

        // Добавляем виджет
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        RemoveWidgetCommand cmd(m_scene, &m_layout, w.id);
        cmd.execute();
        QCOMPARE(m_scene->widgetItems().size(), 0);

        cmd.undo();
        QCOMPARE(m_scene->widgetItems().size(), 1);
        cleanup();
    }

    void testMoveWidgetCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        MoveWidgetCommand cmd(m_scene, &m_layout, w.id,
                              QPointF(10, 20), QPointF(50, 60));
        cmd.execute();

        auto *item = m_scene->widgetItem(w.id);
        QVERIFY(item != nullptr);
        QCOMPARE(item->pos(), QPointF(50, 60));

        cmd.undo();
        QCOMPARE(item->pos(), QPointF(10, 20));
        cleanup();
    }

    void testMoveWidgetMerge()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        MoveWidgetCommand cmd1(m_scene, &m_layout, w.id,
                               QPointF(10, 20), QPointF(30, 40));
        MoveWidgetCommand cmd2(m_scene, &m_layout, w.id,
                               QPointF(30, 40), QPointF(50, 60));

        bool merged = cmd1.mergeWith(&cmd2);
        QVERIFY(merged);

        cmd1.execute();
        auto *item = m_scene->widgetItem(w.id);
        QCOMPARE(item->pos(), QPointF(50, 60));

        cmd1.undo();
        QCOMPARE(item->pos(), QPointF(10, 20));
        cleanup();
    }

    void testResizeWidgetCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        ResizeWidgetCommand cmd(m_scene, &m_layout, w.id,
                                QRectF(10, 20, 100, 40),
                                QRectF(10, 20, 200, 80));
        cmd.execute();

        auto *item = m_scene->widgetItem(w.id);
        QCOMPARE(item->widgetWidth(), 200.0);
        QCOMPARE(item->widgetHeight(), 80.0);

        cmd.undo();
        QCOMPARE(item->widgetWidth(), 100.0);
        QCOMPARE(item->widgetHeight(), 40.0);
        cleanup();
    }

    void testChangePropertyCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);
        w.properties["text"] = "Old";
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        ChangeWidgetPropertyCommand cmd(m_scene, &m_layout, w.id,
                                         "text", "Old", "New");
        cmd.execute();
        auto *item = m_scene->widgetItem(w.id);
        QCOMPARE(item->property("text").toString(), QString("New"));

        cmd.undo();
        QCOMPARE(item->property("text").toString(), QString("Old"));
        cleanup();
    }

    void testBindEventCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 100, 40);
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        BindEventCommand cmd(m_scene, &m_layout, w.id, "onClick", "", "handleClick");
        cmd.execute();

        auto *item = m_scene->widgetItem(w.id);
        QCOMPARE(item->events()["onClick"], QString("handleClick"));

        cmd.undo();
        QVERIFY(!item->events().contains("onClick"));
        cleanup();
    }

    void testChangeLayoutCommand()
    {
        setup();
        UIWidget w = UIWidget::create("Panel", "panel1");
        w.geometry = QRectF(0, 0, 300, 200);
        m_scene->addWidgetItem(w);
        m_layout.window.children.append(w);

        auto *item = m_scene->widgetItem(w.id);
        QCOMPARE(item->layoutType(), QString("None"));

        ChangeLayoutCommand cmd(m_scene, &m_layout, w.id, "None", "HBox");
        cmd.execute();
        QCOMPARE(item->layoutType(), QString("HBox"));

        cmd.undo();
        QCOMPARE(item->layoutType(), QString("None"));
        cleanup();
    }
};

QTEST_MAIN(TestUICommands)
#include "test_UICommands.moc"

// Тесты WidgetItem
#include <QTest>
#include <QApplication>
#include "../../src/uiDesigner/WidgetItem.h"
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestWidgetItem : public QObject {
    Q_OBJECT

private slots:
    void testCreateButton()
    {
        UIWidget w = UIWidget::create("Button", "btn1");
        w.geometry = QRectF(10, 20, 120, 40);
        w.properties["text"] = "Click me";

        WidgetItem item(w);
        QCOMPARE(item.widgetId(), w.id);
        QCOMPARE(item.widgetType(), QString("Button"));
        QCOMPARE(item.widgetName(), QString("btn1"));
        QCOMPARE(item.widgetWidth(), 120.0);
        QCOMPARE(item.widgetHeight(), 40.0);
    }

    void testCreateLabel()
    {
        UIWidget w = UIWidget::create("Label", "lbl1");
        w.geometry = QRectF(0, 0, 100, 30);
        WidgetItem item(w);
        QCOMPARE(item.widgetType(), QString("Label"));
    }

    void testProperties()
    {
        UIWidget w = UIWidget::create("TextField", "tf1");
        w.geometry = QRectF(0, 0, 150, 30);
        WidgetItem item(w);

        item.setWidgetProperty("text", "hello");
        QCOMPARE(item.property("text").toString(), QString("hello"));

        item.setWidgetProperty("placeholder", "Enter text...");
        QCOMPARE(item.property("placeholder").toString(), QString("Enter text..."));
    }

    void testBoundingRect()
    {
        UIWidget w = UIWidget::create("Button", "btn");
        w.geometry = QRectF(0, 0, 100, 40);
        WidgetItem item(w);

        QRectF br = item.boundingRect();
        QVERIFY(br.width() >= 100.0);
        QVERIFY(br.height() >= 40.0);
    }

    void testToFromUIWidget()
    {
        UIWidget w = UIWidget::create("Slider", "sl1");
        w.geometry = QRectF(50, 60, 200, 30);
        w.properties["min"] = 0;
        w.properties["max"] = 100;
        w.properties["value"] = 50;

        WidgetItem item(w);
        UIWidget result = item.toUIWidget();

        QCOMPARE(result.id, w.id);
        QCOMPARE(result.type, QString("Slider"));
        QCOMPARE(result.name, QString("sl1"));
        QCOMPARE(result.properties["min"].toInt(), 0);
        QCOMPARE(result.properties["max"].toInt(), 100);
    }

    void testChildWidgets()
    {
        UIWidget parent = UIWidget::create("Panel", "panel1");
        parent.geometry = QRectF(0, 0, 200, 150);
        UIWidget child = UIWidget::create("Button", "btn_child");
        child.geometry = QRectF(10, 10, 80, 30);

        WidgetItem parentItem(parent);
        WidgetItem *childItem = new WidgetItem(child);
        parentItem.addChildWidget(childItem);

        QCOMPARE(parentItem.childWidgets().size(), 1);
        QCOMPARE(parentItem.childWidgets().first()->widgetId(), child.id);

        parentItem.removeChildWidget(childItem);
        QCOMPARE(parentItem.childWidgets().size(), 0);
    }

    void testSnapToGrid()
    {
        QPointF p(13.0, 27.0);
        QPointF snapped = WidgetItem::snapToGrid(p, 10.0);
        QCOMPARE(snapped, QPointF(10.0, 30.0));

        QPointF p2(25.0, 25.0);
        QPointF snapped2 = WidgetItem::snapToGrid(p2, 10.0);
        QCOMPARE(snapped2, QPointF(30.0, 30.0));
    }

    void testSetWidgetSize()
    {
        UIWidget w = UIWidget::create("Button", "btn");
        w.geometry = QRectF(0, 0, 100, 40);
        WidgetItem item(w);

        item.setWidgetSize(200, 60);
        QCOMPARE(item.widgetWidth(), 200.0);
        QCOMPARE(item.widgetHeight(), 60.0);

        // Минимальный размер
        item.setWidgetSize(5, 5);
        QVERIFY(item.widgetWidth() >= 20.0);
        QVERIFY(item.widgetHeight() >= 20.0);
    }

    void testEvents()
    {
        UIWidget w = UIWidget::create("Button", "btn");
        w.geometry = QRectF(0, 0, 100, 40);
        WidgetItem item(w);

        item.setEvent("onClick", "handleClick");
        QCOMPARE(item.events().size(), 1);
        QCOMPARE(item.events()["onClick"], QString("handleClick"));

        item.removeEvent("onClick");
        QCOMPARE(item.events().size(), 0);
    }
};

QTEST_MAIN(TestWidgetItem)
#include "test_WidgetItem.moc"

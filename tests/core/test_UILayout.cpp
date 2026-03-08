// Тесты UILayout/UIWidget — сериализация, operator==, валидация
#include <QtTest>
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestUILayout : public QObject {
    Q_OBJECT

private slots:
    // --- UIWidget ---

    void widgetCreate()
    {
        auto w = UIWidget::create("Button", "btn_ok");
        QVERIFY(!w.id.isEmpty());
        QCOMPARE(w.type, "Button");
        QCOMPARE(w.name, "btn_ok");
        QCOMPARE(w.layout, "None");
        QCOMPARE(w.contractType(), QString("button"));
        QCOMPARE(w.metadata["deltaq.kind"].toString(), QString("ui_widget_contract"));
    }

    void widgetRoundtrip()
    {
        auto w = UIWidget::create("Panel", "toolbar");
        w.geometry = QRectF(0, 0, 800, 40);
        w.layout = "HBox";
        w.properties["background_color"] = "#2d2d2d";
        w.properties["spacing"] = 5;
        w.events["on_click"] = "handle_click";

        auto json = w.toJson();
        auto restored = UIWidget::fromJson(json);
        QCOMPARE(restored.id, w.id);
        QCOMPARE(restored.type, w.type);
        QCOMPARE(restored.name, w.name);
        QCOMPARE(restored.geometry, w.geometry);
        QCOMPARE(restored.layout, w.layout);
        QCOMPARE(restored.events, w.events);
        QCOMPARE(restored.contractType(), QString("panel"));
        // QVariant из JSON: число может восстановиться как double
        QCOMPARE(restored.properties["background_color"].toString(), "#2d2d2d");
    }

    void widgetRestoresContractMetadataForLegacyJson()
    {
        QJsonObject json;
        json["id"] = "legacy_button";
        json["type"] = "Button";
        json["name"] = "btn_legacy";
        json["x"] = 0;
        json["y"] = 0;
        json["width"] = 120;
        json["height"] = 40;

        auto restored = UIWidget::fromJson(json);
        QCOMPARE(restored.contractType(), QString("button"));
        QCOMPARE(restored.legacyWidgetType(), QString("Button"));
        QCOMPARE(restored.metadata["deltaq.ui.contract_type"].toString(), QString("button"));
    }

    void widgetEquality()
    {
        auto a = UIWidget::create("Label", "lbl1");
        auto b = a; // копия
        QVERIFY(a == b);

        b.name = "lbl2";
        QVERIFY(!(a == b));
    }

    void widgetWithChildren()
    {
        auto parent = UIWidget::create("Panel", "root");
        parent.layout = "VBox";

        auto child1 = UIWidget::create("Button", "btn1");
        child1.geometry = QRectF(0, 0, 100, 30);
        auto child2 = UIWidget::create("Label", "lbl1");
        child2.geometry = QRectF(0, 30, 200, 20);

        parent.children = {child1, child2};

        auto json = parent.toJson();
        auto restored = UIWidget::fromJson(json);
        QCOMPARE(restored.children.size(), 2);
        QCOMPARE(restored.children[0].type, "Button");
        QCOMPARE(restored.children[0].name, "btn1");
        QCOMPARE(restored.children[1].type, "Label");
        QCOMPARE(restored.children[1].name, "lbl1");
    }

    // --- UILayout ---

    void layoutCreate()
    {
        auto l = UILayout::create("MainWindow");
        QVERIFY(!l.id.isEmpty());
        QCOMPARE(l.name, "MainWindow");
        QCOMPARE(l.version, "1.0.0");
        QCOMPARE(l.window.type, "Window");
        QCOMPARE(l.window.geometry, QRectF(0, 0, 800, 600));
        QCOMPARE(l.window.contractType(), QString("window"));
        QCOMPARE(l.metadata["deltaq.kind"].toString(), QString("ui_layout_contract"));
        QCOMPARE(l.metadata["deltaq.ui.backend"].toString(), QString("agnostic"));
    }

    void layoutRoundtrip()
    {
        auto l = UILayout::create("TestLayout");
        auto btn = UIWidget::create("Button", "btn_save");
        btn.geometry = QRectF(10, 10, 80, 30);
        btn.events["on_click"] = "save_handler";
        l.window.children = {btn};

        auto json = l.toJson();
        auto restored = UILayout::fromJson(json);
        QCOMPARE(restored.id, l.id);
        QCOMPARE(restored.name, l.name);
        QCOMPARE(restored.version, l.version);
        QCOMPARE(restored.window.type, "Window");
        QCOMPARE(restored.window.children.size(), 1);
        QCOMPARE(restored.window.children[0].name, "btn_save");
        QCOMPARE(restored.metadata["deltaq.ui.layer"].toString(), QString("contract"));
    }

    void layoutEquality()
    {
        auto a = UILayout::create("A");
        auto b = a; // полная копия
        QVERIFY(a == b);

        b.name = "B";
        QVERIFY(!(a == b));
    }

    void layoutIsValid()
    {
        UILayout l;
        QVERIFY(!l.isValid()); // id и name пусты

        l.id = "id1";
        QVERIFY(!l.isValid()); // name пуст

        l.name = "ok";
        QVERIFY(l.isValid());
    }
};

QTEST_MAIN(TestUILayout)
#include "test_UILayout.moc"

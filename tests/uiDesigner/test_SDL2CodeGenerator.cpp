// Тесты SDL2CodeGenerator
#include <QTest>
#include "../../src/uiDesigner/SDL2CodeGenerator.h"
#include <deltaq/UILayout.h>

using namespace DeltaQ;

class TestSDL2CodeGenerator : public QObject {
    Q_OBJECT

private:
    UILayout createTestLayout()
    {
        UILayout layout = UILayout::create("TestApp");
        layout.window.geometry = QRectF(0, 0, 800, 600);

        UIWidget btn = UIWidget::create("Button", "btnOk");
        btn.geometry = QRectF(10, 10, 120, 40);
        btn.properties["text"] = "OK";
        btn.events["onClick"] = "on_ok_click";
        layout.window.children.append(btn);

        UIWidget lbl = UIWidget::create("Label", "lblTitle");
        lbl.geometry = QRectF(10, 60, 200, 30);
        lbl.properties["text"] = "Hello DeltaQ";
        layout.window.children.append(lbl);

        UIWidget tf = UIWidget::create("TextField", "tfName");
        tf.geometry = QRectF(10, 100, 200, 30);
        tf.properties["placeholder"] = "Enter name";
        layout.window.children.append(tf);

        return layout;
    }

private slots:
    void testGenerateProduces5Files()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(!code.mainFile.isEmpty());
        QVERIFY(!code.uiHeader.isEmpty());
        QVERIFY(!code.uiSource.isEmpty());
        QVERIFY(!code.eventsHeader.isEmpty());
        QVERIFY(!code.eventsSource.isEmpty());
    }

    void testMainFileContainsSDL()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.mainFile.contains("SDL_Init"));
        QVERIFY(code.mainFile.contains("SDL_CreateWindow"));
        QVERIFY(code.mainFile.contains("SDL_CreateRenderer"));
        QVERIFY(code.mainFile.contains("SDL_Quit"));
        QVERIFY(code.mainFile.contains("TTF_Init"));
        QVERIFY(code.mainFile.contains("ui_init"));
        QVERIFY(code.mainFile.contains("ui_render"));
        QVERIFY(code.mainFile.contains("ui_handle_event"));
    }

    void testUIHeaderContainsStructs()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("DQ_Button"));
        QVERIFY(code.uiHeader.contains("DQ_Label"));
        QVERIFY(code.uiHeader.contains("DQ_TextField"));
        QVERIFY(code.uiHeader.contains("UIState"));
        QVERIFY(code.uiHeader.contains("void ui_init"));
    }

    void testUISourceContainsInit()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiSource.contains("ui_init"));
        QVERIFY(code.uiSource.contains("ui_render"));
        QVERIFY(code.uiSource.contains("ui_handle_event"));
        QVERIFY(code.uiSource.contains("btnOk"));
        QVERIFY(code.uiSource.contains("lblTitle"));
        QVERIFY(code.uiSource.contains("tfName"));
    }

    void testEventsHeaderContainsHandlers()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.eventsHeader.contains("on_ok_click"));
    }

    void testEventsSourceContainsHandlers()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.eventsSource.contains("on_ok_click"));
        QVERIFY(code.eventsSource.contains("TODO"));
    }
};

QTEST_APPLESS_MAIN(TestSDL2CodeGenerator)
#include "test_SDL2CodeGenerator.moc"

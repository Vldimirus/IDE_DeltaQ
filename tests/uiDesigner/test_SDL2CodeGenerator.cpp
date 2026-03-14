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

    UILayout createExtendedWidgetLayout()
    {
        UILayout layout = UILayout::create("ExtendedWidgets");
        layout.window.geometry = QRectF(0, 0, 800, 600);

        UIWidget group = UIWidget::create("GroupBox", "groupMain");
        group.geometry = QRectF(20, 20, 320, 180);
        group.properties["text"] = "Main Group";

        UIWidget combo = UIWidget::create("ComboBox", "comboStatus");
        combo.geometry = QRectF(30, 50, 180, 30);
        combo.properties["items"] = "Ready, Busy, Offline";
        combo.properties["selected"] = 1;
        combo.events["onSelectionChanged"] = "on_combo_changed";
        group.children.append(combo);

        UIWidget image = UIWidget::create("Image", "heroImage");
        image.geometry = QRectF(30, 95, 120, 70);
        image.properties["path"] = "assets/logo.png";
        group.children.append(image);

        layout.window.children.append(group);
        return layout;
    }

    UILayout createRemainingWidgetLayout()
    {
        UILayout layout = UILayout::create("RemainingWidgets");
        layout.window.geometry = QRectF(0, 0, 900, 700);

        UIWidget scroll = UIWidget::create("ScrollPanel", "scrollHost");
        scroll.geometry = QRectF(20, 20, 260, 180);
        layout.window.children.append(scroll);

        UIWidget tabs = UIWidget::create("TabPanel", "tabsMain");
        tabs.geometry = QRectF(320, 20, 260, 180);

        UIWidget radioPrimary = UIWidget::create("RadioButton", "radioPrimary");
        radioPrimary.geometry = QRectF(30, 40, 180, 28);
        radioPrimary.properties["text"] = "Primary";
        radioPrimary.properties["selected"] = true;
        radioPrimary.events["onToggled"] = "on_primary_toggled";
        tabs.children.append(radioPrimary);

        UIWidget radioSecondary = UIWidget::create("RadioButton", "radioSecondary");
        radioSecondary.geometry = QRectF(30, 78, 180, 28);
        radioSecondary.properties["text"] = "Secondary";
        radioSecondary.properties["selected"] = false;
        tabs.children.append(radioSecondary);

        layout.window.children.append(tabs);
        return layout;
    }

    UILayout createFinalContractWidgetLayout()
    {
        UILayout layout = UILayout::create("FinalContractWidgets");
        layout.window.geometry = QRectF(0, 0, 1000, 720);

        UIWidget menuBar = UIWidget::create("MenuBar", "menuMain");
        menuBar.geometry = QRectF(0, 0, 1000, 28);
        layout.window.children.append(menuBar);

        UIWidget toolBar = UIWidget::create("ToolBar", "toolsMain");
        toolBar.geometry = QRectF(0, 34, 1000, 32);
        layout.window.children.append(toolBar);

        UIWidget spin = UIWidget::create("SpinBox", "spinCount");
        spin.geometry = QRectF(20, 90, 120, 30);
        spin.properties["min"] = 0;
        spin.properties["max"] = 10;
        spin.properties["value"] = 3;
        spin.events["onValueChanged"] = "on_spin_changed";
        layout.window.children.append(spin);

        UIWidget canvas = UIWidget::create("Canvas", "canvasMain");
        canvas.geometry = QRectF(20, 140, 220, 150);
        layout.window.children.append(canvas);

        UIWidget table = UIWidget::create("Table", "tableMain");
        table.geometry = QRectF(270, 90, 240, 150);
        layout.window.children.append(table);

        UIWidget listView = UIWidget::create("ListView", "listMain");
        listView.geometry = QRectF(540, 90, 180, 150);
        layout.window.children.append(listView);

        UIWidget treeView = UIWidget::create("TreeView", "treeMain");
        treeView.geometry = QRectF(750, 90, 200, 170);
        layout.window.children.append(treeView);

        UIWidget statusBar = UIWidget::create("StatusBar", "statusMain");
        statusBar.geometry = QRectF(0, 688, 1000, 24);
        layout.window.children.append(statusBar);

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

    void testMainFileUsesBackendBoundary()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.mainFile.contains("Generated by DeltaQ IDE."));
        QVERIFY(code.mainFile.contains("Source: UI layout 'TestApp'"));
        QVERIFY(code.mainFile.contains("UI contract layer: contract"));
        QVERIFY(code.mainFile.contains("UI backend: SDL2"));
        QVERIFY(code.mainFile.contains("File role: SDL2 main source (main.c)."));
        QVERIFY(code.mainFile.contains("DQ_UIBackendContext backend;"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_init(&backend"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_poll_event(&event)"));
        QVERIFY(code.mainFile.contains("DQ_UIRuntimeEvent runtime_event;"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_translate_event(&event, &runtime_event)"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_window_size(&backend, &window_width, &window_height);"));
        QVERIFY(code.mainFile.contains("ui_apply_layout(&ui, window_width, window_height);"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_begin_frame(&backend)"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_end_frame(&backend)"));
        QVERIFY(code.mainFile.contains("dq_ui_backend_shutdown(&backend)"));
        QVERIFY(code.mainFile.contains("ui_init"));
        QVERIFY(code.mainFile.contains("ui_render"));
        QVERIFY(code.mainFile.contains("ui_handle_event"));
        QVERIFY(!code.mainFile.contains("SDL_CreateWindow("));
    }

    void testUIHeaderContainsStructs()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("File role: UI header (ui.h)."));
        QVERIFY(code.uiHeader.contains("DQ_Button"));
        QVERIFY(code.uiHeader.contains("DQ_Label"));
        QVERIFY(code.uiHeader.contains("DQ_TextField"));
        QVERIFY(code.uiHeader.contains("UIState"));
        QVERIFY(code.uiHeader.contains("typedef struct {\n    DQ_UIBackendWindow *window;"));
        QVERIFY(code.uiHeader.contains("bool dq_ui_backend_init"));
        QVERIFY(code.uiHeader.contains("typedef SDL_Renderer DQ_UIBackendRenderer;"));
        QVERIFY(code.uiHeader.contains("typedef enum {\n    DQ_UIRuntimeEvent_None = 0,"));
        QVERIFY(code.uiHeader.contains("typedef enum {\n    DQ_UIWidget_None = 0,"));
        QVERIFY(code.uiHeader.contains("typedef enum {\n    DQ_UIRuntimeLayout_None = 0,"));
        QVERIFY(code.uiHeader.contains("typedef struct {\n    int mouse_x;\n    int mouse_y;\n    int window_width;\n    int window_height;\n    DQ_UIWidgetId hovered_widget;"));
        QVERIFY(code.uiHeader.contains("typedef struct {\n    DQ_UIRuntimeLayoutKind kind;\n    int margin;\n    int spacing;\n} DQ_UIRuntimeLayoutSpec;"));
        QVERIFY(code.uiHeader.contains("typedef struct {\n    bool left;\n    bool right;\n    bool top;\n    bool bottom;"));
        QVERIFY(code.uiHeader.contains("DQ_UIRuntimeContext runtime;"));
        QVERIFY(code.uiHeader.contains("void dq_ui_backend_window_size(DQ_UIBackendContext *backend, int *width, int *height);"));
        QVERIFY(code.uiHeader.contains("bool dq_ui_backend_translate_event"));
        QVERIFY(code.uiHeader.contains("void ui_apply_layout(UIState *ui, int window_width, int window_height);"));
        QVERIFY(code.uiHeader.contains("void ui_handle_event(UIState *ui, const DQ_UIRuntimeEvent *event);"));
        QVERIFY(code.uiHeader.contains("void ui_init"));
    }

    void testUISourceContainsInit()
    {
        UILayout layout = createTestLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiSource.contains("ui_init"));
        QVERIFY(code.uiSource.contains("ui_render"));
        QVERIFY(code.uiSource.contains("ui_handle_event"));
        QVERIFY(code.uiSource.contains("bool dq_ui_backend_init(DQ_UIBackendContext *backend"));
        QVERIFY(code.uiSource.contains("void dq_ui_backend_begin_frame(DQ_UIBackendContext *backend)"));
        QVERIFY(code.uiSource.contains("bool dq_ui_backend_translate_event(const DQ_UIBackendEvent *backend_event,"));
        QVERIFY(code.uiSource.contains("void dq_ui_backend_window_size(DQ_UIBackendContext *backend, int *width, int *height)"));
        QVERIFY(code.uiSource.contains("static bool dq_ui_runtime_try_open_bundled_font(UIState *ui, int size)"));
        QVERIFY(code.uiSource.contains("getenv(\"DELTAQ_FONT_PATH\")"));
        QVERIFY(code.uiSource.contains("getenv(\"DELTAQ_ASSET_ROOT\")"));
        QVERIFY(code.uiSource.contains("../assets/fonts/default.ttf"));
        QVERIFY(code.uiSource.contains("static DQ_UIWidgetId dq_ui_runtime_hit_test(const UIState *ui, int x, int y)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_sync_state(UIState *ui)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_dispatch_click(UIState *ui, DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static SDL_Rect dq_ui_runtime_rect_from_base(const SDL_Rect *parent_rect,"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_apply_anchor_spec(SDL_Rect *rect,"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_apply_hbox(const SDL_Rect *parent_rect,"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_apply_layout_spec(const SDL_Rect *parent_rect,"));
        QVERIFY(code.uiSource.contains("static const DQ_UIRuntimeLayoutSpec dq_ui_layoutspec_root = {"));
        QVERIFY(code.uiSource.contains("static const DQ_UIRuntimeAnchorSpec dq_ui_anchorspec_btnOk = {"));
        QVERIFY(code.uiSource.contains("void ui_apply_layout(UIState *ui, int window_width, int window_height)"));
        QVERIFY(code.uiSource.contains("ui->runtime.hovered_widget = dq_ui_runtime_hit_test(ui, event->mouse_x, event->mouse_y);"));
        QVERIFY(code.uiSource.contains("case DQ_UIRuntimeEvent_TextInput:"));
        QVERIFY(code.uiSource.contains("dq_ui_runtime_append_text(ui, ui->runtime.focused_widget, event->text);"));
        QVERIFY(code.uiSource.contains("dq_ui_runtime_sync_state(ui);"));
        QVERIFY(code.uiSource.contains("case DQ_UIRuntimeEvent_MouseDown:"));
        QVERIFY(code.uiSource.contains("dq_ui_runtime_try_open_bundled_font(ui, 14);"));
        QVERIFY(code.uiSource.contains("btnOk"));
        QVERIFY(code.uiSource.contains("lblTitle"));
        QVERIFY(code.uiSource.contains("tfName"));
    }

    void testContractMetadataDrivesWidgetGeneration()
    {
        UILayout layout = UILayout::create("ContractDriven");

        UIWidget tf = UIWidget::create("TextField", "tfContract");
        tf.type = "LegacyTextField";
        tf.properties["placeholder"] = "Contract placeholder";
        layout.window.children.append(tf);

        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("DQ_LegacyTextField"));
        QVERIFY(code.uiSource.contains("ui->tfContract.placeholder = \"Contract placeholder\";"));
        QVERIFY(code.uiSource.contains("ui->tfContract.focused = ui->runtime.focused_widget == DQ_UIWidget_tfContract;"));
    }

    void testWindowMetadataDrivesBackendInit()
    {
        UILayout layout = UILayout::create("WindowContract");
        layout.window.geometry = QRectF(0, 0, 320, 240);
        layout.window.properties["title"] = "Stage1 Window";
        layout.window.properties["min_width"] = 420;
        layout.window.properties["min_height"] = 310;
        layout.window.properties["resizable"] = false;

        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.mainFile.contains("dq_ui_backend_init(&backend, \"Stage1 Window\", 420, 310, 420, 310, false)"));
        QVERIFY(code.uiSource.contains("Uint32 window_flags = SDL_WINDOW_SHOWN;"));
        QVERIFY(code.uiSource.contains("if (resizable) {"));
        QVERIFY(code.uiSource.contains("window_flags |= SDL_WINDOW_RESIZABLE;"));
        QVERIFY(code.uiSource.contains("SDL_SetWindowMinimumSize(backend->window, min_width, min_height);"));
        QVERIFY(code.uiSource.contains("ui_apply_layout(ui, 420, 310);"));
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

        QVERIFY(code.eventsSource.contains("File role: event implementation (ui_events.c)."));
        QVERIFY(code.eventsSource.contains("on_ok_click"));
        QVERIFY(code.eventsSource.contains("TODO"));
    }

    void testExtendedContractWidgetsUseExplicitRuntimeBranches()
    {
        UILayout layout = createExtendedWidgetLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("DQ_GroupBox"));
        QVERIFY(code.uiHeader.contains("DQ_ComboBox"));
        QVERIFY(code.uiHeader.contains("DQ_Image"));
        QVERIFY(code.uiHeader.contains("const char *items;"));
        QVERIFY(code.uiHeader.contains("char display_text[128];"));
        QVERIFY(code.uiHeader.contains("const char *path;"));

        QVERIFY(code.uiSource.contains("static bool dq_ui_runtime_widget_is_combo_box(DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static int dq_ui_runtime_csv_item_count(const char *items)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_cycle_combo_box(UIState *ui, DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("ui->groupMain.text = \"Main Group\";"));
        QVERIFY(code.uiSource.contains("ui->comboStatus.items = \"Ready, Busy, Offline\";"));
        QVERIFY(code.uiSource.contains("ui->comboStatus.selected = 1;"));
        QVERIFY(code.uiSource.contains("dq_ui_runtime_cycle_combo_box(ui, DQ_UIWidget_comboStatus);"));
        QVERIFY(code.uiSource.contains("on_combo_changed(ui);"));
        QVERIFY(code.uiSource.contains("ui->heroImage.path = \"assets/logo.png\";"));
        QVERIFY(code.uiSource.contains("render_groupbox"));
        QVERIFY(code.uiSource.contains("render_combobox"));
        QVERIFY(code.uiSource.contains("render_image"));
        QVERIFY(!code.uiHeader.contains("TODO: GroupBox fields"));
        QVERIFY(!code.uiHeader.contains("TODO: ComboBox fields"));
        QVERIFY(!code.uiHeader.contains("TODO: Image fields"));
        QVERIFY(!code.uiSource.contains("TODO: implement GroupBox rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement ComboBox rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement Image rendering"));
    }

    void testRemainingContractWidgetsUseExplicitRuntimeBranches()
    {
        UILayout layout = createRemainingWidgetLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("DQ_ScrollPanel"));
        QVERIFY(code.uiHeader.contains("DQ_TabPanel"));
        QVERIFY(code.uiHeader.contains("DQ_RadioButton"));
        QVERIFY(code.uiHeader.contains("int scroll_y;"));
        QVERIFY(code.uiHeader.contains("int active_tab;"));
        QVERIFY(code.uiHeader.contains("bool selected;"));

        QVERIFY(code.uiSource.contains("static bool dq_ui_runtime_widget_is_radio_button(DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_dispatch_toggle(UIState *ui, DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_select_radio_button(UIState *ui, DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("ui->scrollHost.scroll_y = 0;"));
        QVERIFY(code.uiSource.contains("ui->tabsMain.active_tab = 0;"));
        QVERIFY(code.uiSource.contains("ui->radioPrimary.selected = true;"));
        QVERIFY(code.uiSource.contains("ui->radioSecondary.selected = false;"));
        QVERIFY(code.uiSource.contains("ui->radioPrimary.text = \"Primary\";"));
        QVERIFY(code.uiSource.contains("render_scrollpanel"));
        QVERIFY(code.uiSource.contains("render_tabpanel"));
        QVERIFY(code.uiSource.contains("render_radiobutton"));
        QVERIFY(code.uiSource.contains("on_primary_toggled(ui);"));
        QVERIFY(!code.uiHeader.contains("TODO: ScrollPanel fields"));
        QVERIFY(!code.uiHeader.contains("TODO: TabPanel fields"));
        QVERIFY(!code.uiHeader.contains("TODO: RadioButton fields"));
        QVERIFY(!code.uiSource.contains("TODO: implement ScrollPanel rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement TabPanel rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement RadioButton rendering"));
    }

    void testFinalContractWidgetsUseExplicitRuntimeBranches()
    {
        UILayout layout = createFinalContractWidgetLayout();
        GeneratedCode code = SDL2CodeGenerator::generate(layout);

        QVERIFY(code.uiHeader.contains("DQ_SpinBox"));
        QVERIFY(code.uiHeader.contains("DQ_Canvas"));
        QVERIFY(code.uiHeader.contains("DQ_Table"));
        QVERIFY(code.uiHeader.contains("DQ_ListView"));
        QVERIFY(code.uiHeader.contains("DQ_TreeView"));
        QVERIFY(code.uiHeader.contains("DQ_MenuBar"));
        QVERIFY(code.uiHeader.contains("DQ_ToolBar"));
        QVERIFY(code.uiHeader.contains("DQ_StatusBar"));
        QVERIFY(code.uiHeader.contains("int min_val, max_val, value;"));

        QVERIFY(code.uiSource.contains("static bool dq_ui_runtime_widget_is_spin_box(DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_dispatch_value_changed(UIState *ui, DQ_UIWidgetId widget_id)"));
        QVERIFY(code.uiSource.contains("static void dq_ui_runtime_step_spin_box(UIState *ui, DQ_UIWidgetId widget_id, int mouse_x)"));
        QVERIFY(code.uiSource.contains("ui->spinCount.min_val = 0;"));
        QVERIFY(code.uiSource.contains("ui->spinCount.max_val = 10;"));
        QVERIFY(code.uiSource.contains("ui->spinCount.value = 3;"));
        QVERIFY(code.uiSource.contains("on_spin_changed(ui);"));
        QVERIFY(code.uiSource.contains("render_spinbox"));
        QVERIFY(code.uiSource.contains("render_canvas"));
        QVERIFY(code.uiSource.contains("render_table"));
        QVERIFY(code.uiSource.contains("render_listview"));
        QVERIFY(code.uiSource.contains("render_treeview"));
        QVERIFY(code.uiSource.contains("render_menubar"));
        QVERIFY(code.uiSource.contains("render_toolbar"));
        QVERIFY(code.uiSource.contains("render_statusbar"));
        QVERIFY(!code.uiHeader.contains("TODO: SpinBox fields"));
        QVERIFY(!code.uiHeader.contains("TODO: Canvas fields"));
        QVERIFY(!code.uiHeader.contains("TODO: Table fields"));
        QVERIFY(!code.uiHeader.contains("TODO: ListView fields"));
        QVERIFY(!code.uiHeader.contains("TODO: TreeView fields"));
        QVERIFY(!code.uiHeader.contains("TODO: MenuBar fields"));
        QVERIFY(!code.uiHeader.contains("TODO: ToolBar fields"));
        QVERIFY(!code.uiHeader.contains("TODO: StatusBar fields"));
        QVERIFY(!code.uiSource.contains("TODO: implement SpinBox rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement Canvas rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement Table rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement ListView rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement TreeView rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement MenuBar rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement ToolBar rendering"));
        QVERIFY(!code.uiSource.contains("TODO: implement StatusBar rendering"));
    }
};

QTEST_APPLESS_MAIN(TestSDL2CodeGenerator)
#include "test_SDL2CodeGenerator.moc"

// Тесты ModulePalette — видимость и доступность модулей в палитре
#include <QtTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QCheckBox>
#include "../../src/blockEditor/ModulePalette.h"
#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/StandardLibrary.h"

using namespace DeltaQ;

class TestModulePalette : public QObject {
    Q_OBJECT

private:
    ModuleRegistry *m_registry = nullptr;
    GraphStore *m_graphStore = nullptr;

    // Регистрирует модуль с минимально достаточным контрактом для палитры.
    void registerModule(const QString &id, const QString &name,
                        const QString &origin, const QString &testStatus)
    {
        Module mod;
        mod.id = id;
        mod.name = name;
        mod.origin = origin;
        mod.language = "c";
        mod.version = "1.0";
        mod.inputs = {{"value", "int", "0"}};
        mod.outputs = {{"result", "int", ""}};
        mod.sourceCode = "int dq_module(int value) {\n"
                         "    return value;\n"
                         "}";
        mod.compileStatus = (testStatus == "passed") ? "passed" : "unknown";
        mod.testStatus = testStatus;
        QVERIFY(m_registry->registerModule(mod));
    }

    // Ищет leaf-элемент дерева по moduleId, независимо от секции и категории.
    static QTreeWidgetItem *findModuleItem(QTreeWidget *tree, const QString &moduleId)
    {
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            auto *item = *it;
            if (item->data(0, Qt::UserRole).toString() == moduleId)
                return item;
            ++it;
        }
        return nullptr;
    }

private slots:
    void init()
    {
        m_registry = new ModuleRegistry(this);
        m_graphStore = new GraphStore(this);
        m_registry->setGraphStore(m_graphStore);
    }

    void cleanup()
    {
        delete m_graphStore;
        m_graphStore = nullptr;
        delete m_registry;
        m_registry = nullptr;
    }

    void paletteMarksDraftModulesAsUnavailable()
    {
        registerModule("ready", "ReadyModule", "local", "passed");
        registerModule("draft", "DraftModule", "local", "untested");

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        QVERIFY(tree != nullptr);

        auto *readyItem = findModuleItem(tree, "ready");
        auto *draftItem = findModuleItem(tree, "draft");
        QVERIFY(readyItem != nullptr);
        QVERIFY(draftItem != nullptr);

        QVERIFY(readyItem->flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!draftItem->flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(!readyItem->text(0).contains(QString::fromUtf8("[не готов]")));
        QVERIFY(draftItem->text(0).contains(QString::fromUtf8("[не готов]")));
        QVERIFY(readyItem->toolTip(0).contains(QString::fromUtf8("готов к использованию")));
        QVERIFY(draftItem->toolTip(0).contains(QString::fromUtf8("не готов к использованию")));
        QVERIFY(draftItem->toolTip(0).contains(QString::fromUtf8("compile check passes")));
    }

    void paletteMarksBrokenCompositeModuleAsUnavailable()
    {
        Module composite = Module::create("BrokenComposite", "c");
        composite.origin = "graph";
        composite.category = "composite";
        composite.graphId = "missing_graph";
        QVERIFY(m_registry->registerModule(composite));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        QVERIFY(tree != nullptr);

        auto *compositeItem = findModuleItem(tree, composite.id);
        QVERIFY(compositeItem != nullptr);

        QVERIFY(!compositeItem->flags().testFlag(Qt::ItemIsDragEnabled));
        QVERIFY(compositeItem->text(0).contains(QString::fromUtf8("[не готов]")));
        QVERIFY(compositeItem->toolTip(0).contains(QString::fromUtf8("inner graph")));
    }

    void paletteUsesStandardLibraryOrder()
    {
        Module control = Module::create("sequence", "c");
        control.id = "core.control.sequence";
        control.origin = "core";
        control.category = "control";
        control.sourceCode = "// flow\n";
        control.compileStatus = "passed";
        control.testStatus = "passed";
        QVERIFY(m_registry->registerModule(control));

        Module println = Module::create("println", "c");
        println.id = "core.io.println";
        println.origin = "core";
        println.category = "io";
        println.sourceCode = "void dq_println(const char *text) { (void)text; }\n";
        println.compileStatus = "passed";
        println.testStatus = "passed";
        QVERIFY(m_registry->registerModule(println));

        Module print = Module::create("print", "c");
        print.id = "core.io.print";
        print.origin = "core";
        print.category = "io";
        print.sourceCode = "void dq_print(const char *text) { (void)text; }\n";
        print.compileStatus = "passed";
        print.testStatus = "passed";
        QVERIFY(m_registry->registerModule(print));

        Module math = Module::create("add", "c");
        math.id = "core.math.add";
        math.origin = "core";
        math.category = "math";
        math.sourceCode = "int dq_add(int a, int b) { return a + b; }\n";
        math.compileStatus = "passed";
        math.testStatus = "passed";
        QVERIFY(m_registry->registerModule(math));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        QVERIFY(tree != nullptr);
        QVERIFY(tree->topLevelItemCount() > 0);

        QTreeWidgetItem *coreRoot = tree->topLevelItem(0);
        QVERIFY(coreRoot != nullptr);
        QCOMPARE(coreRoot->text(0), QString("Стандартная библиотека"));
        QCOMPARE(coreRoot->childCount(), 3);
        QCOMPARE(coreRoot->child(0)->text(0), StandardLibrary::requiredV1Categories().at(0));
        QCOMPARE(coreRoot->child(1)->text(0), StandardLibrary::requiredV1Categories().at(1));
        QCOMPARE(coreRoot->child(2)->text(0), StandardLibrary::requiredV1Categories().at(5));

        QTreeWidgetItem *ioCategory = coreRoot->child(1);
        QVERIFY(ioCategory != nullptr);
        QCOMPARE(ioCategory->childCount(), 2);
        QCOMPARE(ioCategory->child(0)->text(0), QString("print"));
        QCOMPARE(ioCategory->child(1)->text(0), QString("println"));
        QVERIFY(ioCategory->child(0)->toolTip(0).contains(QString::fromUtf8("Опорный модуль v1")));
    }

    void paletteShowsCoreCurationHints()
    {
        Module printInt = Module::create("print_int", "c");
        printInt.id = "core.io.print_int";
        printInt.origin = "core";
        printInt.category = "io";
        printInt.sourceCode = "void dq_print_int(int value) { (void)value; }\n";
        printInt.compileStatus = "passed";
        printInt.testStatus = "passed";
        QVERIFY(m_registry->registerModule(printInt));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        auto *showLegacy = palette.findChild<QCheckBox *>("modulePaletteShowLegacyCheck");
        QVERIFY(tree != nullptr);
        QVERIFY(showLegacy != nullptr);

        auto *item = findModuleItem(tree, "core.io.print_int");
        QVERIFY(item == nullptr);

        showLegacy->setChecked(true);
        auto *legacyItem = findModuleItem(tree, "core.io.print_int");
        QVERIFY(legacyItem != nullptr);
        QCOMPARE(legacyItem->text(0), QString("print_int [legacy]"));
        QVERIFY(legacyItem->toolTip(0).contains(QString("Legacy shortcut")));
        QVERIFY(legacyItem->toolTip(0).contains(QString("int_to_string")));
    }

    void paletteHidesLegacyCoreModulesByDefault()
    {
        Module printInt = Module::create("print_int", "c");
        printInt.id = "core.io.print_int";
        printInt.origin = "core";
        printInt.category = "io";
        printInt.sourceCode = "void dq_print_int(int value) { (void)value; }\n";
        printInt.compileStatus = "passed";
        printInt.testStatus = "passed";
        QVERIFY(m_registry->registerModule(printInt));

        Module print = Module::create("print", "c");
        print.id = "core.io.print";
        print.origin = "core";
        print.category = "io";
        print.sourceCode = "void dq_print(const char *text) { (void)text; }\n";
        print.compileStatus = "passed";
        print.testStatus = "passed";
        QVERIFY(m_registry->registerModule(print));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        auto *showLegacy = palette.findChild<QCheckBox *>("modulePaletteShowLegacyCheck");
        QVERIFY(tree != nullptr);
        QVERIFY(showLegacy != nullptr);

        QVERIFY(findModuleItem(tree, "core.io.print_int") == nullptr);
        QVERIFY(findModuleItem(tree, "core.io.print") != nullptr);

        showLegacy->setChecked(true);
        QVERIFY(findModuleItem(tree, "core.io.print_int") != nullptr);
    }

    void paletteHidesSpecializedCoreModulesByDefault()
    {
        Module compare = Module::create("str_compare", "c");
        compare.id = "core.string.str_compare";
        compare.origin = "core";
        compare.category = "string";
        compare.sourceCode = "int dq_str_compare(const char *a, const char *b) { return a == b; }\n";
        compare.compileStatus = "passed";
        compare.testStatus = "passed";
        QVERIFY(m_registry->registerModule(compare));

        Module add = Module::create("add", "c");
        add.id = "core.math.add";
        add.origin = "core";
        add.category = "math";
        add.sourceCode = "int dq_add(int a, int b) { return a + b; }\n";
        add.compileStatus = "passed";
        add.testStatus = "passed";
        QVERIFY(m_registry->registerModule(add));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        auto *showSpecialized = palette.findChild<QCheckBox *>("modulePaletteShowSpecializedCheck");
        QVERIFY(tree != nullptr);
        QVERIFY(showSpecialized != nullptr);

        QVERIFY(findModuleItem(tree, "core.string.str_compare") == nullptr);
        QVERIFY(findModuleItem(tree, "core.math.add") != nullptr);

        QTreeWidgetItem *coreRoot = tree->topLevelItem(0);
        QVERIFY(coreRoot != nullptr);
        bool hasStringCategory = false;
        for (int i = 0; i < coreRoot->childCount(); ++i) {
            if (!coreRoot->child(i)->isHidden() && coreRoot->child(i)->text(0) == "string")
                hasStringCategory = true;
        }
        QVERIFY(!hasStringCategory);

        showSpecialized->setChecked(true);
        QVERIFY(findModuleItem(tree, "core.string.str_compare") != nullptr);
    }

    void paletteHidesHiddenImportedWrappersAndShowsCuratedLabels()
    {
        Module visible = Module::create("mini_sensor_read", "c");
        visible.id = "ext.mini_sensor_sdk_raw.mini_sensor_read";
        visible.origin = "extension";
        visible.category = "sensor_raw";
        visible.sourceCode = "int dq_mini_sensor_read(void *ctx) { return mini_sensor_read(ctx); }\n";
        visible.compileStatus = "passed";
        visible.testStatus = "passed";
        visible.metadata["deltaq.import.kind"] = "library_pack_module";
        visible.metadata["deltaq.import.pack_name"] = "mini_sensor_sdk_raw";
        visible.metadata["deltaq.import.original_symbol"] = "mini_sensor_read";
        visible.metadata["deltaq.import.display_name"] = "Read Sensor";
        visible.metadata["deltaq.import.curation_role"] = "raw_wrapper";
        QVERIFY(m_registry->registerModule(visible));

        Module hidden = visible;
        hidden.id = "ext.mini_sensor_sdk_raw.mini_sensor_last_error";
        hidden.name = "mini_sensor_last_error";
        hidden.metadata["deltaq.import.original_symbol"] = "mini_sensor_last_error";
        hidden.metadata["deltaq.import.display_name"] = "Last Error";
        hidden.metadata["deltaq.import.curation_role"] = "hidden";
        QVERIFY(m_registry->registerModule(hidden));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        QVERIFY(tree != nullptr);

        auto *visibleItem = findModuleItem(tree, visible.id);
        auto *hiddenItem = findModuleItem(tree, hidden.id);
        QVERIFY(visibleItem != nullptr);
        QCOMPARE(visibleItem->text(0), QString("Read Sensor [raw]"));
        QVERIFY(visibleItem->toolTip(0).contains(QString("Imported pack: mini_sensor_sdk_raw")));
        QVERIFY(visibleItem->toolTip(0).contains(QString::fromUtf8("Символ: mini_sensor_read")));
        QVERIFY(visibleItem->toolTip(0).contains(QString::fromUtf8("Роль import/curation: raw")));
        QVERIFY(hiddenItem == nullptr);
    }

    void paletteShowsDocumentationBlocks()
    {
        Module documented = Module::create("sensor_scale_report", "c");
        documented.id = "ext.mini_sensor_sdk_curated.sensor_scale_report";
        documented.origin = "extension";
        documented.category = "sensor_curated";
        documented.sourceCode = "int dq_sensor_scale_report(const char *device_name, int factor) { return factor; }\n";
        documented.compileStatus = "passed";
        documented.testStatus = "passed";
        documented.description = "Curated imported entry for sensor scale report";
        documented.metadata["deltaq.import.kind"] = "library_pack_module";
        documented.metadata["deltaq.import.pack_name"] = "mini_sensor_sdk_curated";
        documented.metadata["deltaq.import.original_symbol"] = "sensor_scale_report";
        documented.metadata["deltaq.import.display_name"] = "Sensor Scale Report";
        documented.metadata["deltaq.import.curation_role"] = "curated_entry";
        documented.metadata["deltaq.doc.when_to_use"] =
            QString::fromUtf8("Когда нужен готовый success-path imported SDK без raw wrapper-ов.");
        documented.metadata["deltaq.doc.limitations"] =
            QString::fromUtf8("Печатает runtime report и рассчитан на fixture mini_sensor_sdk.");
        QVERIFY(m_registry->registerModule(documented));

        ModulePalette palette(m_registry);
        auto *tree = palette.findChild<QTreeWidget *>("modulePaletteTree");
        QVERIFY(tree != nullptr);

        auto *item = findModuleItem(tree, documented.id);
        QVERIFY(item != nullptr);
        QVERIFY(item->toolTip(0).contains(QString::fromUtf8("Назначение: Curated imported entry for sensor scale report")));
        QVERIFY(item->toolTip(0).contains(QString::fromUtf8("Когда использовать: Когда нужен готовый success-path imported SDK без raw wrapper-ов.")));
        QVERIFY(item->toolTip(0).contains(QString::fromUtf8("Ограничения: Печатает runtime report и рассчитан на fixture mini_sensor_sdk.")));
    }
};

QTEST_MAIN(TestModulePalette)
#include "test_ModulePalette.moc"

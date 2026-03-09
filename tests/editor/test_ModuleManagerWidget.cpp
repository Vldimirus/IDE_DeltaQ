#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTest>
#include <functional>

#include <deltaq/Module.h>

#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/StandardLibrary.h"
#include "../../src/editor/ModuleManagerWidget.h"

using namespace DeltaQ;

class TestModuleManagerWidget : public QObject {
    Q_OBJECT

private:
    // Создаёт минимальный модуль для проверки статусов в Module Manager.
    static Module makeModule(const QString &name, const QString &origin,
                             const QString &sourceCode,
                             const QString &compileStatus = "unknown",
                             const QString &testStatus = "untested")
    {
        Module mod = Module::create(name, "c");
        mod.origin = origin;
        mod.category = "custom";
        mod.sourceCode = sourceCode;
        mod.compileStatus = compileStatus;
        mod.testStatus = testStatus;
        return mod;
    }

    static void processUi()
    {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }

    // Ищет leaf-элемент дерева по moduleId, независимо от секции и категории.
    static QTreeWidgetItem *findModuleItem(QTreeWidget *tree, const QString &moduleId)
    {
        std::function<QTreeWidgetItem *(QTreeWidgetItem *)> findItem;
        findItem = [&](QTreeWidgetItem *parent) -> QTreeWidgetItem * {
            for (int i = 0; i < parent->childCount(); ++i) {
                auto *child = parent->child(i);
                if (child->data(0, Qt::UserRole).toString() == moduleId)
                    return child;
                if (auto *nested = findItem(child))
                    return nested;
            }
            return nullptr;
        };

        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            if (auto *item = findItem(tree->topLevelItem(i)))
                return item;
        }
        return nullptr;
    }

    static QTreeWidgetItem *findItemByText(QTreeWidget *tree, const QString &text)
    {
        std::function<QTreeWidgetItem *(QTreeWidgetItem *)> findItem;
        findItem = [&](QTreeWidgetItem *parent) -> QTreeWidgetItem * {
            if (parent->text(0) == text)
                return parent;
            for (int i = 0; i < parent->childCount(); ++i) {
                if (auto *nested = findItem(parent->child(i)))
                    return nested;
            }
            return nullptr;
        };

        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            if (auto *item = findItem(tree->topLevelItem(i)))
                return item;
        }
        return nullptr;
    }

private slots:
    void localModuleWithoutImplementationIsMarkedNotReady()
    {
        ModuleRegistry registry;
        Module mod = makeModule("draft_module", "local", "");
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *badge = widget.findChild<QLabel *>("moduleStateBadge");
        auto *details = widget.findChild<QLabel *>("moduleStateDetails");
        auto *layer = widget.findChild<QLabel *>("moduleEcosystemLayerLabel");
        auto *role = widget.findChild<QLabel *>("moduleEcosystemRoleLabel");
        auto *quality = widget.findChild<QLabel *>("moduleEcosystemQualityLabel");
        QVERIFY(badge != nullptr);
        QVERIFY(details != nullptr);
        QVERIFY(layer != nullptr);
        QVERIFY(role != nullptr);
        QVERIFY(quality != nullptr);

        QCOMPARE(badge->text(), QString("Неготов"));
        QVERIFY(details->text().contains(QString("Контракт: корректен")));
        QVERIFY(details->text().contains(QString("Реализация: отсутствует")));
        QVERIFY(details->text().contains(QString("Компиляция: не запускалась")));
        QVERIFY(details->text().contains(QString("Тест: не запускался")));
        QCOMPARE(layer->text(), QString("Проектный модуль"));
        QVERIFY(role->text().contains(QString::fromUtf8("локальный атомарный")));
        QVERIFY(quality->text().contains(QString("staged verification")));
    }

    void coreModuleShowsLibraryState()
    {
        ModuleRegistry registry;
        Module mod = makeModule("core_ready", "core", "int dq_core_ready(void) {\n    return 1;\n}\n");
        mod.id = "core.io.print";
        mod.name = "print";
        mod.category = "io";
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *badge = widget.findChild<QLabel *>("moduleStateBadge");
        auto *details = widget.findChild<QLabel *>("moduleStateDetails");
        auto *layer = widget.findChild<QLabel *>("moduleEcosystemLayerLabel");
        auto *role = widget.findChild<QLabel *>("moduleEcosystemRoleLabel");
        auto *quality = widget.findChild<QLabel *>("moduleEcosystemQualityLabel");
        QVERIFY(badge != nullptr);
        QVERIFY(details != nullptr);
        QVERIFY(layer != nullptr);
        QVERIFY(role != nullptr);
        QVERIFY(quality != nullptr);

        QCOMPARE(badge->text(), QString("Библиотечный"));
        QVERIFY(details->text().contains(QString("Режим: только чтение")));
        QCOMPARE(layer->text(), QString("Стандартная библиотека"));
        QVERIFY(role->text().contains(QString::fromUtf8("Опорный модуль v1")));
        QVERIFY(quality->text().contains(QString("explicit curation review")));

        auto *tree = widget.findChild<QTreeWidget *>();
        QVERIFY(tree != nullptr);
        QTreeWidgetItem *found = findModuleItem(tree, mod.id);
        QVERIFY(found != nullptr);
        QVERIFY(found->toolTip(0).contains(QString::fromUtf8("Опорный модуль v1")));
    }

    void legacyCoreModuleShowsLegacyHint()
    {
        ModuleRegistry registry;
        Module mod = makeModule("print_int", "core", "void dq_print_int(int value) { (void)value; }\n");
        mod.id = "core.io.print_int";
        mod.category = "io";
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *tree = widget.findChild<QTreeWidget *>();
        auto *showLegacy = widget.findChild<QCheckBox *>("moduleManagerShowLegacyCheck");
        QVERIFY(tree != nullptr);
        QVERIFY(showLegacy != nullptr);

        QVERIFY(findModuleItem(tree, mod.id) == nullptr);

        showLegacy->setChecked(true);
        processUi();

        QTreeWidgetItem *found = findModuleItem(tree, mod.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->text(0), QString("print_int [legacy]"));
        QVERIFY(found->toolTip(0).contains(QString("Legacy shortcut")));
        QVERIFY(found->toolTip(0).contains(QString("int_to_string")));
    }

    void specializedCoreModuleStaysOutOfDefaultBaseline()
    {
        ModuleRegistry registry;

        Module specialized = makeModule("str_compare", "core",
            "int dq_str_compare(const char *a, const char *b) {\n    return a == b;\n}\n");
        specialized.id = "core.string.str_compare";
        specialized.category = "string";
        QVERIFY(registry.registerModule(specialized));

        Module essential = makeModule("add", "core",
            "int dq_add(int a, int b) {\n    return a + b;\n}\n");
        essential.id = "core.math.add";
        essential.category = "math";
        QVERIFY(registry.registerModule(essential));

        ModuleManagerWidget widget(&registry);
        auto *tree = widget.findChild<QTreeWidget *>();
        auto *showSpecialized = widget.findChild<QCheckBox *>("moduleManagerShowSpecializedCheck");
        QVERIFY(tree != nullptr);
        QVERIFY(showSpecialized != nullptr);

        QVERIFY(findModuleItem(tree, specialized.id) == nullptr);
        QVERIFY(findModuleItem(tree, essential.id) != nullptr);

        showSpecialized->setChecked(true);
        processUi();

        QVERIFY(findModuleItem(tree, specialized.id) != nullptr);
    }

    void passedModuleStaysPassedOnOpenAndBecomesModifiedAfterEdit()
    {
        ModuleRegistry registry;
        Module mod = makeModule("checked_module", "local",
                                "int dq_checked_module(int value) {\n    return value + 1;\n}\n",
                                "passed",
                                "passed");
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *badge = widget.findChild<QLabel *>("moduleStateBadge");
        auto *details = widget.findChild<QLabel *>("moduleStateDetails");
        auto *codeEdit = widget.findChild<QPlainTextEdit *>("moduleCodeEditor");
        QVERIFY(badge != nullptr);
        QVERIFY(details != nullptr);
        QVERIFY(codeEdit != nullptr);

        // Простое открытие не должно сбрасывать already passed статус.
        QCOMPARE(badge->text(), QString("Проверен"));
        QCOMPARE(registry.findModule(mod.id)->compileStatus, QString("passed"));
        QCOMPARE(registry.findModule(mod.id)->testStatus, QString("passed"));

        codeEdit->setPlainText(codeEdit->toPlainText() + "\n");
        processUi();

        QCOMPARE(registry.findModule(mod.id)->compileStatus, QString("modified"));
        QCOMPARE(registry.findModule(mod.id)->testStatus, QString("modified"));
        QCOMPARE(badge->text(), QString("Изменён"));
        QVERIFY(details->text().contains(QString("Компиляция: изменён после компиляции")));
        QVERIFY(details->text().contains(QString("Тест: изменён после проверки")));
    }

    void brokenCompositeModuleIsMarkedNotReady()
    {
        ModuleRegistry registry;
        GraphStore graphStore;
        registry.setGraphStore(&graphStore);

        Module mod = makeModule("broken_composite", "graph", "");
        mod.graphId = "missing_graph";
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *badge = widget.findChild<QLabel *>("moduleStateBadge");
        auto *details = widget.findChild<QLabel *>("moduleStateDetails");
        QVERIFY(badge != nullptr);
        QVERIFY(details != nullptr);

        QCOMPARE(badge->text(), QString("Неготов"));
        QVERIFY(details->text().contains(QString("Тип: составной")));
        QVERIFY(details->text().contains(QString("Реализация: внутренний граф")));
        QVERIFY(details->text().contains(QString("Допуск: нет")));
        QVERIFY(details->text().contains(QString("inner graph")));
    }

    void compiledButUntestedModuleRequiresVerification()
    {
        ModuleRegistry registry;
        Module mod = makeModule("compiled_only", "local",
                                "int dq_compiled_only(int value) {\n    return value;\n}\n",
                                "passed",
                                "untested");
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        QVERIFY(widget.openModule(mod.id));
        processUi();

        auto *badge = widget.findChild<QLabel *>("moduleStateBadge");
        auto *details = widget.findChild<QLabel *>("moduleStateDetails");
        QVERIFY(badge != nullptr);
        QVERIFY(details != nullptr);

        QCOMPARE(badge->text(), QString("Требует проверки"));
        QVERIFY(details->text().contains(QString("Компиляция: пройдена")));
        QVERIFY(details->text().contains(QString("Тест: не запускался")));
    }

    void uiModuleTreeUsesDisplayNameAndContractType()
    {
        ModuleRegistry registry;
        Module mod = makeModule("UI_Button", "ui", "// ui contract");
        mod.category = "ui";
        mod.metadata["deltaq.kind"] = "ui_contract";
        mod.metadata["deltaq.ui.display_name"] = "Button";
        mod.metadata["deltaq.ui.widget_type"] = "button";
        mod.description = "Контракт кнопки DeltaQ";
        QVERIFY(registry.registerModule(mod));

        ModuleManagerWidget widget(&registry);
        processUi();

        auto *tree = widget.findChild<QTreeWidget *>();
        QVERIFY(tree != nullptr);
        QTreeWidgetItem *found = findModuleItem(tree, mod.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->text(0), QString("Button [button]"));
        QVERIFY(found->toolTip(0).contains(QString("Контракт кнопки DeltaQ")));
    }

    void coreCategoriesFollowStandardLibraryOrder()
    {
        ModuleRegistry registry;

        Module math = makeModule("add", "core", "int dq_add(int a, int b) { return a + b; }\n");
        math.category = "math";
        QVERIFY(registry.registerModule(math));

        Module io = makeModule("print", "core", "void dq_print(const char *text) { (void)text; }\n");
        io.category = "io";
        io.id = "core.io.print";
        QVERIFY(registry.registerModule(io));

        Module ioSecond = makeModule("println", "core", "void dq_println(const char *text) { (void)text; }\n");
        ioSecond.category = "io";
        ioSecond.id = "core.io.println";
        QVERIFY(registry.registerModule(ioSecond));

        Module control = makeModule("sequence", "core", "// flow\n");
        control.category = "control";
        control.id = "core.control.sequence";
        QVERIFY(registry.registerModule(control));

        ModuleManagerWidget widget(&registry);
        processUi();

        auto *tree = widget.findChild<QTreeWidget *>();
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
    }

    void importedModuleSaveWritesBackToOriginalPackFile()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString modulesRoot = tempDir.path() + "/modules";
        const QString packDir = modulesRoot + "/mini_sensor_sdk_raw";
        const QString categoryDir = packDir + "/sensor_raw";
        QVERIFY(QDir().mkpath(categoryDir));

        QFile packFile(packDir + "/pack.json");
        QVERIFY(packFile.open(QIODevice::WriteOnly | QIODevice::Text));
        packFile.write("{\n  \"name\": \"Mini Sensor SDK Raw\",\n  \"version\": \"1.0.0\"\n}\n");
        packFile.close();

        Module imported = makeModule("mini_sensor_read", "extension",
            "int dq_mini_sensor_read(void *ctx) {\n    return mini_sensor_read(ctx);\n}\n",
            "passed",
            "passed");
        imported.id = "ext.mini_sensor_sdk_raw.mini_sensor_read";
        imported.category = "sensor_raw";
        imported.metadata["deltaq.import.kind"] = "library_pack_module";
        imported.metadata["deltaq.import.pack_name"] = "mini_sensor_sdk_raw";
        imported.metadata["deltaq.import.original_symbol"] = "mini_sensor_read";
        imported.metadata["deltaq.import.display_name"] = "Read Sensor";
        imported.metadata["deltaq.import.curation_role"] = "raw_wrapper";
        imported.metadata["deltaq.doc.when_to_use"] = QString::fromUtf8("Использовать как curated imported module.");
        imported.metadata["deltaq.doc.limitations"] = QString::fromUtf8("Зависит от mini_sensor_sdk fixture.");

        const QString modulePath = categoryDir + "/mini_sensor_read.dqmod";
        QFile moduleFile(modulePath);
        QVERIFY(moduleFile.open(QIODevice::WriteOnly | QIODevice::Text));
        moduleFile.write(QJsonDocument(imported.toJson()).toJson(QJsonDocument::Indented));
        moduleFile.close();

        ModuleRegistry registry;
        registry.loadGlobalModules(modulesRoot);

        ModuleManagerWidget widget(&registry);
        widget.setGlobalModulesDir(modulesRoot);
        QVERIFY(widget.openModule(imported.id));
        processUi();

        auto *displayNameEdit = widget.findChild<QLineEdit *>("importDisplayNameEdit");
        auto *roleCombo = widget.findChild<QComboBox *>("importRoleCombo");
        auto *whenToUseEdit = widget.findChild<QTextEdit *>("moduleDocWhenToUseEdit");
        auto *limitationsEdit = widget.findChild<QTextEdit *>("moduleDocLimitationsEdit");
        auto *layer = widget.findChild<QLabel *>("moduleEcosystemLayerLabel");
        auto *role = widget.findChild<QLabel *>("moduleEcosystemRoleLabel");
        auto *quality = widget.findChild<QLabel *>("moduleEcosystemQualityLabel");
        auto *tree = widget.findChild<QTreeWidget *>();
        QVERIFY(displayNameEdit != nullptr);
        QVERIFY(roleCombo != nullptr);
        QVERIFY(whenToUseEdit != nullptr);
        QVERIFY(limitationsEdit != nullptr);
        QVERIFY(layer != nullptr);
        QVERIFY(role != nullptr);
        QVERIFY(quality != nullptr);
        QVERIFY(tree != nullptr);

        QCOMPARE(whenToUseEdit->toPlainText(), QString::fromUtf8("Использовать как curated imported module."));
        QCOMPARE(limitationsEdit->toPlainText(), QString::fromUtf8("Зависит от mini_sensor_sdk fixture."));
        QCOMPARE(layer->text(), QString("Imported pack"));
        QVERIFY(role->text().contains(QString("raw")));
        QVERIFY(role->text().contains(QString("mini_sensor_sdk_raw")));
        QVERIFY(quality->text().contains(QString("mini_sensor_read")));
        QVERIFY(quality->text().contains(QString("compile")));

        displayNameEdit->setText("Sensor Read");
        roleCombo->setCurrentIndex(roleCombo->findData("adapter"));
        whenToUseEdit->setPlainText(QString::fromUtf8("Когда нужно быстро проверить imported SDK в графе."));
        limitationsEdit->setPlainText(QString::fromUtf8("Показывает low-level imported поведение без дополнительной валидации."));
        processUi();

        QVERIFY(QMetaObject::invokeMethod(&widget, "onSaveModule", Qt::DirectConnection));
        processUi();

        const Module *saved = registry.findModule(imported.id);
        QVERIFY(saved != nullptr);
        QCOMPARE(saved->storagePath, modulePath);
        QCOMPARE(saved->metadataString("deltaq.import.display_name"), QString("Sensor Read"));
        QCOMPARE(saved->metadataString("deltaq.import.curation_role"), QString("adapter"));
        QCOMPARE(saved->documentationWhenToUse(),
                 QString::fromUtf8("Когда нужно быстро проверить imported SDK в графе."));
        QCOMPARE(saved->documentationLimitations(),
                 QString::fromUtf8("Показывает low-level imported поведение без дополнительной валидации."));

        QFile savedFile(modulePath);
        QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QJsonObject savedJson = QJsonDocument::fromJson(savedFile.readAll()).object();
        const QJsonObject metadata = savedJson["metadata"].toObject();
        QCOMPARE(metadata["deltaq.import.display_name"].toString(), QString("Sensor Read"));
        QCOMPARE(metadata["deltaq.import.curation_role"].toString(), QString("adapter"));
        QCOMPARE(metadata["deltaq.doc.when_to_use"].toString(),
                 QString::fromUtf8("Когда нужно быстро проверить imported SDK в графе."));
        QCOMPARE(metadata["deltaq.doc.limitations"].toString(),
                 QString::fromUtf8("Показывает low-level imported поведение без дополнительной валидации."));

        QTreeWidgetItem *found = findModuleItem(tree, imported.id);
        QVERIFY(found != nullptr);
        QCOMPARE(found->text(0), QString("Sensor Read [adapter]"));
        QVERIFY(found->toolTip(0).contains(QString::fromUtf8("Роль import/curation: adapter")));
        QVERIFY(found->toolTip(0).contains(QString::fromUtf8("Когда использовать: Когда нужно быстро проверить imported SDK в графе.")));
        QVERIFY(found->toolTip(0).contains(QString::fromUtf8("Ограничения: Показывает low-level imported поведение без дополнительной валидации.")));
        QVERIFY2(role->text().contains(QString("adapter")), qPrintable(role->text()));
    }

    void importedModulesAppearUnderDedicatedPackSection()
    {
        ModuleRegistry registry;

        Module imported = makeModule("mini_sensor_read", "extension",
            "int dq_mini_sensor_read(void *ctx) {\n    return mini_sensor_read(ctx);\n}\n",
            "passed",
            "passed");
        imported.id = "ext.mini_sensor_sdk_raw.mini_sensor_read";
        imported.category = "sensor_raw";
        imported.metadata["deltaq.import.kind"] = "library_pack_module";
        imported.metadata["deltaq.import.pack_name"] = "mini_sensor_sdk_raw";
        imported.metadata["deltaq.import.original_symbol"] = "mini_sensor_read";
        imported.metadata["deltaq.import.display_name"] = "Read Sensor";
        imported.metadata["deltaq.import.curation_role"] = "raw_wrapper";
        QVERIFY(registry.registerModule(imported));

        Module extension = makeModule("fft_window", "extension",
            "int dq_fft_window(int value) {\n    return value;\n}\n",
            "passed",
            "passed");
        extension.id = "ext.signal_tools.fft_window";
        extension.category = "signal";
        QVERIFY(registry.registerModule(extension));

        ModuleManagerWidget widget(&registry);
        processUi();

        auto *tree = widget.findChild<QTreeWidget *>();
        QVERIFY(tree != nullptr);

        QTreeWidgetItem *importedItem = findModuleItem(tree, imported.id);
        QTreeWidgetItem *extensionItem = findModuleItem(tree, extension.id);
        QTreeWidgetItem *packItem = findItemByText(tree, "mini_sensor_sdk_raw");
        QVERIFY(importedItem != nullptr);
        QVERIFY(extensionItem != nullptr);
        QVERIFY(packItem != nullptr);

        QCOMPARE(importedItem->parent()->text(0), QString("sensor_raw"));
        QCOMPARE(importedItem->parent()->parent()->text(0), QString("mini_sensor_sdk_raw"));
        QCOMPARE(importedItem->parent()->parent()->parent()->text(0), QString::fromUtf8("Импортированные пакеты"));
        QCOMPARE(extensionItem->parent()->text(0), QString("signal"));
        QCOMPARE(extensionItem->parent()->parent()->text(0), QString::fromUtf8("Расширения"));
        QVERIFY(packItem->toolTip(0).contains(QString("Imported pack: mini_sensor_sdk_raw")));
        QVERIFY(packItem->toolTip(0).contains(QString("Raw: 1")));
    }
};

QTEST_MAIN(TestModuleManagerWidget)
#include "test_ModuleManagerWidget.moc"

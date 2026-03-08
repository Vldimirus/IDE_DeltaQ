// Тесты UIModuleFactory — UI contract-модули и их metadata
#include <QTest>
#include "../../src/uiDesigner/UIModuleFactory.h"

using namespace DeltaQ;

class TestUIModuleFactory : public QObject {
    Q_OBJECT

private:
    const Module *findModule(const QVector<Module> &modules, const QString &id) const
    {
        for (const auto &module : modules) {
            if (module.id == id)
                return &module;
        }
        return nullptr;
    }

private slots:
    void createsBackendAgnosticContractModules()
    {
        const QVector<Module> modules = UIModuleFactory::createUIModules();
        const Module *button = findModule(modules, "ui_module_button");
        QVERIFY(button != nullptr);
        QCOMPARE(button->origin, QString("ui"));
        QCOMPARE(button->category, QString("ui"));
        QVERIFY(UIModuleFactory::isUIContractModule(*button));
        QCOMPARE(button->metadataString("deltaq.ui.layer"), QString("contract"));
        QCOMPARE(button->metadataString("deltaq.ui.backend"), QString("agnostic"));
        QCOMPARE(UIModuleFactory::widgetType(*button), QString("button"));
        QCOMPARE(button->metadataString("deltaq.ui.legacy_widget_type"), QString("Button"));
        QVERIFY(button->sourceCode.contains("UI contract module"));
        QVERIFY(!button->sourceCode.contains("SDL_"));
    }

    void widgetTypeSupportsLegacyFallback()
    {
        Module legacy;
        legacy.id = "ui_module_label";
        legacy.name = "UI_Label";
        legacy.origin = "ui";

        QCOMPARE(UIModuleFactory::widgetType(legacy), QString("label"));
        QVERIFY(UIModuleFactory::isUIContractModule(legacy));
    }

    void factoryUsesSharedCatalogForRegisteredModules()
    {
        const QVector<Module> modules = UIModuleFactory::createUIModules();
        QCOMPARE(modules.size(), 8);

        const Module *comboBox = findModule(modules, "ui_module_combobox");
        QVERIFY(comboBox != nullptr);
        QCOMPARE(comboBox->metadataString("deltaq.ui.widget_type"), QString("combo_box"));
        QCOMPARE(comboBox->metadataString("deltaq.ui.display_name"), QString("Combo Box"));
        QCOMPARE(comboBox->outputs.size(), 2);
        QCOMPARE(comboBox->outputs[0].name, QString("onSelectionChanged"));
        QCOMPARE(comboBox->outputs[1].name, QString("selected"));
    }
};

QTEST_APPLESS_MAIN(TestUIModuleFactory)
#include "test_UIModuleFactory.moc"

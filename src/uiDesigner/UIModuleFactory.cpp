// Фабрика UI-модулей — реализация
#include "UIModuleFactory.h"
#include "../core/ModuleRegistry.h"

#include <QJsonArray>
#include <QTextStream>

namespace DeltaQ {

namespace {
QStringList jsonArrayToStringList(const QJsonArray &array)
{
    QStringList result;
    for (const auto &value : array)
        result.append(value.toString());
    return result;
}

// Заполняет contract metadata по портам, чтобы UI runtime/backend мог опираться
// на явное описание свойств, событий и state-выходов, а не на SDL2-фрагменты.
void finalizeContractMetadata(Module &module)
{
    QJsonArray propertyPorts;
    for (const auto &input : module.inputs)
        propertyPorts.append(input.name);

    QJsonArray eventPorts;
    QJsonArray stateOutputs;
    for (const auto &output : module.outputs) {
        if (output.type == "signal")
            eventPorts.append(output.name);
        else
            stateOutputs.append(output.name);
    }

    module.metadata["deltaq.kind"] = "ui_contract";
    module.metadata["deltaq.ui.layer"] = "contract";
    module.metadata["deltaq.ui.backend"] = "agnostic";
    module.metadata["deltaq.ui.contract_version"] = "1.0";
    module.metadata["deltaq.ui.properties"] = propertyPorts;
    if (!eventPorts.isEmpty())
        module.metadata["deltaq.ui.events"] = eventPorts;
    if (!stateOutputs.isEmpty())
        module.metadata["deltaq.ui.state_outputs"] = stateOutputs;

    QString contractText;
    QTextStream out(&contractText);
    out << "// UI contract module: " << module.name << "\n";
    out << "// Этот модуль описывает свойства и события виджета DeltaQ.\n";
    out << "// Конкретная реализация рендера и обработки ввода предоставляется UI backend-ом.\n";
    out << "// Backend v1: SDL2.\n";
    out << "// Widget type: " << module.metadata.value("deltaq.ui.widget_type").toString() << "\n";
    out << "// Legacy widget type: " << module.metadata.value("deltaq.ui.legacy_widget_type").toString() << "\n";
    if (!propertyPorts.isEmpty())
        out << "// Properties: " << jsonArrayToStringList(propertyPorts).join(", ") << "\n";
    if (!eventPorts.isEmpty())
        out << "// Events: " << jsonArrayToStringList(eventPorts).join(", ") << "\n";
    if (!stateOutputs.isEmpty())
        out << "// State outputs: " << jsonArrayToStringList(stateOutputs).join(", ") << "\n";

    module.sourceCode = contractText;
    module.includes.clear();
}

} // namespace

QVector<Module> UIModuleFactory::createUIModules()
{
    QVector<Module> modules;
    // Собираем только те контракты, которые действительно должны быть доступны как модули графа.
    for (const auto &spec : uiContractCatalog()) {
        if (!spec.exposeAsModule || spec.moduleId.isEmpty())
            continue;
        modules.append(createContractModule(spec));
    }
    return modules;
}

void UIModuleFactory::registerAll(ModuleRegistry *registry)
{
    if (!registry)
        return;

    const auto modules = createUIModules();
    for (const auto &mod : modules) {
        // UI contract-модули регистрируем один раз как встроенную библиотеку.
        if (!registry->findModule(mod.id))
            registry->registerModule(mod);
    }
}

bool UIModuleFactory::isUIModule(const QString &moduleId)
{
    // Сохраняем совместимость по прежнему ID-префиксу.
    return moduleId.startsWith("ui_module_");
}

bool UIModuleFactory::isUIModuleByName(const QString &moduleName)
{
    const auto *spec = findUIContractSpecByAny(moduleName);
    return spec != nullptr && spec->exposeAsModule;
}

bool UIModuleFactory::isUIContractModule(const Module &module)
{
    if (module.isUIContractModule())
        return true;

    // Старые проекты могли сохранить UI-модуль без metadata.
    return !widgetType(module).isEmpty() && module.origin == "ui";
}

QString UIModuleFactory::widgetType(const Module &module)
{
    if (!module.uiWidgetType().isEmpty())
        return module.uiWidgetType();

    if (const auto *spec = findUIContractSpecByAny(module.id))
        return spec->contractType;
    if (const auto *spec = findUIContractSpecByAny(module.name))
        return spec->contractType;
    return {};
}

Module UIModuleFactory::createContractModule(const UIContractSpec &spec)
{
    Module module;
    module.id = spec.moduleId;
    module.name = spec.moduleName;
    module.version = "1.0.0";
    module.language = "c";
    module.description = spec.description;
    module.category = "ui";
    module.origin = "ui";
    module.compileStatus = "passed";
    module.testStatus = "passed";
    module.metadata["deltaq.ui.widget_type"] = spec.contractType;
    module.metadata["deltaq.ui.legacy_widget_type"] = spec.legacyWidgetType;
    module.metadata["deltaq.ui.display_name"] = spec.displayName;
    module.metadata["deltaq.ui.palette_category"] = spec.paletteCategory;

    // Порты модуля берём из того же UI-контракта, что и палитра/.dqui, чтобы не было
    // расхождения между тем, что пользователь рисует, и тем, что он подключает в графе.
    for (const auto &input : spec.moduleInputs)
        module.inputs.append({input.name, input.type, input.defaultValue});
    for (const auto &output : spec.moduleOutputs)
        module.outputs.append({output.name, output.type, output.defaultValue});

    finalizeContractMetadata(module);
    return module;
}

} // namespace DeltaQ

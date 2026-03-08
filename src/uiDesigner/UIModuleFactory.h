// Фабрика UI-модулей — генерация Module объектов для виджетов UI
#pragma once

#include <QVector>
#include <deltaq/UIContract.h>
#include <deltaq/Module.h>

namespace DeltaQ {

class ModuleRegistry;

class UIModuleFactory {
public:
    // Создать все UI-модули и вернуть список
    static QVector<Module> createUIModules();

    // Зарегистрировать все UI-модули в реестре
    static void registerAll(ModuleRegistry *registry);

    // Проверить, является ли модуль UI-модулем
    static bool isUIModule(const QString &moduleId);
    static bool isUIModuleByName(const QString &moduleName);
    static bool isUIContractModule(const Module &module);
    static QString widgetType(const Module &module);

private:
    static Module createContractModule(const UIContractSpec &spec);
};

} // namespace DeltaQ

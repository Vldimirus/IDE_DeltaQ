// Фабрика UI-модулей — генерация Module объектов для виджетов UI
#pragma once

#include <QVector>
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

private:
    static Module createButton();
    static Module createTextField();
    static Module createLabel();
    static Module createSlider();
    static Module createCheckbox();
    static Module createProgressBar();
    static Module createImage();
    static Module createComboBox();
};

} // namespace DeltaQ

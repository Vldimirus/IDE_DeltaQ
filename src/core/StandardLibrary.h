// Стандартная библиотека модулей DeltaQ
#pragma once

#include <QVector>
#include <QString>
#include <QStringList>
#include <deltaq/Module.h>

namespace DeltaQ {

struct StandardLibraryAuditReport {
    QStringList requiredCategories;
    QStringList presentCategories;
    QStringList missingCategories;
    QStringList requiredModuleIds;
    QStringList missingModuleIds;
    QStringList issues;

    bool isHealthy() const {
        return missingCategories.isEmpty()
            && missingModuleIds.isEmpty()
            && issues.isEmpty();
    }
};

struct StandardLibraryCurationInfo {
    QString tier;               // essential | convenience | specialized | legacy
    QString title;              // Короткая человекочитаемая роль внутри standard library
    QString guidance;           // Как модуль рекомендуется использовать
    QString replacementHint;    // Подсказка по предпочтительной альтернативе

    bool isKnown() const {
        return !tier.isEmpty();
    }
};

class StandardLibrary {
public:
    // Загружает текущий core pack из source-of-truth `.dqmod` файлов на диске.
    // Название сохранено ради совместимости со старым API, но source-of-truth
    // для библиотеки — checked-in `modules/core`, а не hardcoded C++-описания.
    static QVector<Module> createAll();

    // Обязательные категории standard library для v1.
    static QStringList requiredV1Categories();

    // Обязательные core-модули, без которых v1 библиотека считается неполной.
    static QStringList requiredV1ModuleIds();

    // Product-curation роли core-модулей внутри стандартной библиотеки.
    static StandardLibraryCurationInfo curationForModule(const Module &module);

    // Возвращает категории в product-defined v1 порядке с алфавитным хвостом для расширений.
    static QStringList orderCategoriesForDisplay(const QStringList &categories);

    // Возвращает core-модули в display-порядке: сначала ключевые v1 модули, затем алфавитно.
    static QVector<const Module *> orderModulesForDisplay(const QVector<const Module *> &modules);

    // Формальный аудит quality bar для standard library.
    static StandardLibraryAuditReport audit(const QVector<Module> &modules);

    // Копирует source-of-truth core pack в целевую директорию.
    static void install(const QString &coreDir);

    // Копирует все checked-in official pack-и в writable modules root.
    static void installBundledPacks(const QString &modulesRootDir);

    // Проверяет, совпадает ли версия установленного pack-а с source-of-truth pack-ом.
    static bool isUpToDate(const QString &coreDir);
};

} // namespace DeltaQ

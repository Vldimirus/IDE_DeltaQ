// Декомпозиция ParseResult в модули DeltaQ
#pragma once

#include "LibclangTypes.h"
#include "LibclangParser.h"
#include <deltaq/Module.h>
#include <QStringList>

namespace DeltaQ {

// Настройки декомпозиции
struct DecompositionOptions {
    QString category = "imported";      // категория модулей
    QString language = "c";             // "c" или "cpp"
    QStringList excludePatterns;        // паттерны для исключения (regex)
    bool includePrivateMethods = false; // включать private-методы C++ классов
    bool generateWrappers = true;       // генерировать C-обёртки для C++
};

// Результат декомпозиции
struct DecompositionResult {
    QVector<Module> modules;
    QStringList warnings;
};

class LibraryDecomposer {
public:
    // Декомпозиция результата парсинга в модули
    static DecompositionResult decompose(const ParseResult &parseResult,
                                          const DecompositionOptions &options = {});

private:
    // C-функция → 1 Module
    static Module functionToModule(const FunctionDecl &func,
                                    const DecompositionOptions &options);

    // C++ класс → N модулей (create, destroy, per-method)
    static QVector<Module> classToModules(const ClassDecl &cls,
                                           const DecompositionOptions &options);

    // Проверка паттернов исключения
    static bool isExcluded(const QString &name, const QStringList &patterns);
};

} // namespace DeltaQ

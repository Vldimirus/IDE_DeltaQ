// Стандартная библиотека модулей DeltaQ
#pragma once

#include <QVector>
#include <QString>
#include <deltaq/Module.h>

namespace DeltaQ {

class StandardLibrary {
public:
    // Создать все core-модули в памяти
    static QVector<Module> createAll();

    // Установить .dqmod файлы в coreDir (если версия изменилась)
    static void install(const QString &coreDir);

    // Проверить, установлена ли текущая версия
    static bool isUpToDate(const QString &coreDir);

    static constexpr const char *VERSION = "1.1";
};

} // namespace DeltaQ

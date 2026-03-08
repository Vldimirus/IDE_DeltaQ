// Упаковка импортированной библиотеки в extension pack DeltaQ
#pragma once

#include "WrapperGenerator.h"

#include <deltaq/Module.h>

#include <QStringList>

namespace DeltaQ {

struct ImportedLibraryPackSpec {
    QString packName;
    QString displayName;
    QString version = "1.0";
    QString author = "User";
    QString description;
    QString category = "imported";
    QString language = "c";
    QString standard = "c17";
    QStringList headerPaths;
    QStringList includePaths;
    QStringList defines;
    QStringList linkLibraries;
};

struct ImportedLibraryPackResult {
    QString packDir;
    QStringList writtenModuleFiles;
    QStringList writtenWrapperFiles;
    QStringList warnings;
    QStringList errors;

    bool success() const { return errors.isEmpty(); }
};

class LibraryPackager {
public:
    // Приводит пользовательское имя pack-а к безопасной форме для пути и id namespace.
    static QString sanitizePackName(const QString &name);

    // Формирует extension pack на диске и подготавливает модули к загрузке в ModuleRegistry.
    static ImportedLibraryPackResult writeImportedPack(const QString &modulesRootDir,
                                                       const ImportedLibraryPackSpec &spec,
                                                       const QVector<Module> &modules,
                                                       const QVector<WrapperCode> &wrappers = {});

private:
    static Module prepareModuleForImportedPack(const Module &module,
                                               const ImportedLibraryPackSpec &spec,
                                               QStringList *warnings);
};

} // namespace DeltaQ

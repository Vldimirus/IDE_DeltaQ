// Упаковка импортированной библиотеки в extension pack DeltaQ — реализация
#include "LibraryPackager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>
#include <QTextStream>

namespace DeltaQ {

namespace {

// Делает безопасный сегмент для module id и имён файлов внутри imported pack.
QString sanitizeSegment(const QString &value)
{
    QString result = value.trimmed().toLower();
    for (QChar &ch : result) {
        if (!ch.isLetterOrNumber())
            ch = '_';
    }
    while (result.contains("__"))
        result.replace("__", "_");
    result.remove(QRegularExpression("^_+"));
    result.remove(QRegularExpression("_+$"));
    if (result.isEmpty())
        result = "item";
    if (result.front().isDigit())
        result.prepend("m_");
    return result;
}

// Маппинг DeltaQ-типов в минимально совместимые C-типы для generated wrapper-кода.
QString importedModuleCType(const QString &type)
{
    if (type == "int")
        return "int";
    if (type == "float")
        return "float";
    if (type == "double")
        return "double";
    if (type == "bool")
        return "int";
    if (type == "string")
        return "const char *";
    if (type == "pointer")
        return "void *";
    return "int";
}

// Формирует wrapper sourceCode для простого C-модуля, который просто проксирует вызов внешней функции.
QString makeCFunctionWrapperSource(const Module &module,
                                   const QString &symbolName,
                                   QStringList *warnings)
{
    if (module.isComposite()) {
        if (warnings)
            warnings->append(QObject::tr("composite imported module '%1' was left without source wrapper")
                                 .arg(module.name));
        return {};
    }

    if (module.outputs.size() > 1) {
        if (warnings) {
            warnings->append(QObject::tr("module '%1' has more than one output and was kept as contract-only")
                                 .arg(module.name));
        }
        return {};
    }

    QStringList args;
    QStringList callArgs;
    for (const auto &input : module.inputs) {
        if (input.kind == PortKind::Execution)
            continue;

        const QString argName = sanitizeSegment(input.name);
        args.append(QString("%1 %2").arg(importedModuleCType(input.type), argName));
        callArgs.append(argName);
    }

    const QString functionName = "dq_" + sanitizeSegment(module.name);
    const QString cReturnType = module.outputs.isEmpty()
        ? QString("void")
        : importedModuleCType(module.outputs.first().type);

    QString source;
    QTextStream out(&source);
    out << cReturnType << " " << functionName << "("
        << (args.isEmpty() ? QString("void") : args.join(", ")) << ") {\n";
    if (module.outputs.isEmpty())
        out << "    " << symbolName << "(" << callArgs.join(", ") << ");\n";
    else
        out << "    return " << symbolName << "(" << callArgs.join(", ") << ");\n";
    out << "}";
    return source;
}

// Переносит строки в JSON-массив без дублирования однотипного кода.
QJsonArray toJsonArray(const QStringList &values)
{
    QJsonArray array;
    for (const auto &value : values)
        array.append(value);
    return array;
}

bool looksLikeFilesystemPath(const QString &value)
{
    if (value.isEmpty())
        return false;

    const QFileInfo info(value);
    return info.isAbsolute()
        || value.contains('/')
        || value.contains('\\')
        || value.startsWith('.');
}

bool looksLikeBinaryArtifactPath(const QString &value)
{
    if (looksLikeFilesystemPath(value))
        return true;

    const QString lower = value.trimmed().toLower();
    return lower.endsWith(".a")
        || lower.endsWith(".so")
        || lower.endsWith(".dylib")
        || lower.endsWith(".dll")
        || lower.endsWith(".lib");
}

QString makeStagingDirName(const QString &packName)
{
    return QString(".dq_import_%1_%2")
        .arg(packName)
        .arg(QString::number(QDateTime::currentMSecsSinceEpoch()));
}

} // namespace

QString LibraryPackager::sanitizePackName(const QString &name)
{
    return sanitizeSegment(name);
}

QStringList LibraryPackager::validateImportedPackSpec(const ImportedLibraryPackSpec &spec,
                                                      const QVector<Module> &modules,
                                                      const QVector<WrapperCode> &wrappers)
{
    QStringList errors;

    const QString packName = sanitizePackName(spec.packName);
    if (packName.isEmpty()) {
        errors.append(QObject::tr("Pack name is empty. Next step: enter a stable pack name for the imported library."));
    }

    if (spec.language.trimmed() != "c") {
        errors.append(QObject::tr("Imported pack v1 currently supports Linux-first C ABI only. Next step: expose a thin extern \"C\" adapter or import a C header instead of '%1'.")
                          .arg(spec.language.trimmed().isEmpty() ? QObject::tr("an unspecified ABI")
                                                                 : spec.language.trimmed()));
    }

    const QString standard = spec.standard.trimmed().toLower();
    if (!standard.isEmpty() && (standard.startsWith("c++") || (!standard.startsWith("c11") && !standard.startsWith("c17")))) {
        errors.append(QObject::tr("Standard '%1' is outside the v1 intake scope. Next step: switch to a C standard such as c11/c17 or wrap the library behind a C ABI facade.")
                          .arg(spec.standard.trimmed()));
    }

    if (spec.headerPaths.isEmpty()) {
        errors.append(QObject::tr("No header files were provided. Next step: choose at least one readable .h file for import."));
    }

    for (const auto &headerPath : spec.headerPaths) {
        if (!looksLikeFilesystemPath(headerPath))
            continue;
        if (!QFileInfo::exists(headerPath)) {
            errors.append(QObject::tr("Header file '%1' was not found. Next step: pick an existing header or use an include-style name together with include paths.")
                              .arg(QDir::fromNativeSeparators(headerPath)));
        }
    }

    for (const auto &libraryRef : spec.linkLibraries) {
        if (!looksLikeBinaryArtifactPath(libraryRef))
            continue;
        if (!QFileInfo::exists(libraryRef)) {
            errors.append(QObject::tr("Library artifact '%1' was not found. Next step: build/copy the .so/.a first or switch to a toolchain-resolved library name.")
                              .arg(QDir::fromNativeSeparators(libraryRef)));
        }
    }

    if (modules.isEmpty() && wrappers.isEmpty()) {
        errors.append(QObject::tr("Nothing was selected for import. Next step: keep at least one generated module or wrapper before writing the pack."));
    }

    return errors;
}

Module LibraryPackager::prepareModuleForImportedPack(const Module &module,
                                                     const ImportedLibraryPackSpec &spec,
                                                     QStringList *warnings)
{
    Module prepared = module;
    const QString packName = sanitizePackName(spec.packName);
    const QString symbolName = prepared.name;

    // Imported pack должен давать стабильные ids, иначе переимпорт одного и того же API
    // будет плодить новые UUID вместо обновления существующих модулей.
    prepared.id = QString("ext.%1.%2").arg(packName, sanitizeSegment(prepared.name));
    prepared.origin = "extension";
    prepared.version = spec.version;
    if (!spec.category.trimmed().isEmpty())
        prepared.category = sanitizeSegment(spec.category);

    prepared.metadata["deltaq.import.kind"] = "library_pack_module";
    prepared.metadata["deltaq.import.pack_name"] = packName;
    prepared.metadata["deltaq.import.pack_title"] = spec.displayName;
    prepared.metadata["deltaq.import.language"] = spec.language;
    prepared.metadata["deltaq.import.standard"] = spec.standard;
    prepared.metadata["deltaq.import.original_symbol"] = symbolName;
    prepared.metadata["deltaq.import.display_name"] = symbolName;
    prepared.metadata["deltaq.import.curation_role"] = "raw_wrapper";
    prepared.metadata["deltaq.doc.when_to_use"] =
        QObject::tr("Использовать как raw wrapper при curation imported pack-а "
                    "или для точечного low-level доступа к символу библиотеки.");
    prepared.metadata["deltaq.doc.limitations"] =
        QObject::tr("Низкоуровневый imported wrapper: может напрямую отражать "
                    "внешний API без дополнительной адаптации под граф.");
    prepared.metadata["deltaq.import.header_paths"] = toJsonArray(spec.headerPaths);
    prepared.metadata["deltaq.import.include_paths"] = toJsonArray(spec.includePaths);
    prepared.metadata["deltaq.import.defines"] = toJsonArray(spec.defines);
    prepared.metadata["deltaq.import.link_libraries"] = toJsonArray(spec.linkLibraries);

    // Imported raw wrapper сразу получает минимальное human-readable назначение,
    // чтобы его можно было отличить в палитре и Module Manager без открытия кода.
    if (prepared.description.trimmed().isEmpty())
        prepared.description = QObject::tr("Imported wrapper for '%1'").arg(symbolName);

    prepared.includes.clear();
    for (const auto &headerPath : spec.headerPaths) {
        const QString normalized = QDir::fromNativeSeparators(headerPath);
        prepared.includes.append(QString("\"%1\"").arg(normalized));
    }

    // MVP для C-библиотек: модуль получает собственный wrapper sourceCode и становится
    // полноценным атомарным модулем, а не только контрактом без реализации.
    if (prepared.language == "c")
        prepared.sourceCode = makeCFunctionWrapperSource(prepared, symbolName, warnings);

    return prepared;
}

ImportedLibraryPackResult LibraryPackager::writeImportedPack(const QString &modulesRootDir,
                                                             const ImportedLibraryPackSpec &spec,
                                                             const QVector<Module> &modules,
                                                             const QVector<WrapperCode> &wrappers)
{
    ImportedLibraryPackResult result;

    const QString packName = sanitizePackName(spec.packName);
    result.errors = validateImportedPackSpec(spec, modules, wrappers);
    if (!result.errors.isEmpty())
        return result;

    QDir rootDir(modulesRootDir);
    if (!rootDir.exists() && !QDir().mkpath(modulesRootDir)) {
        result.errors.append(QObject::tr("failed to create modules root '%1'").arg(modulesRootDir));
        return result;
    }
    rootDir = QDir(modulesRootDir);

    const QString packDir = rootDir.filePath(packName);
    result.packDir = packDir;

    QString stagingBaseName = makeStagingDirName(packName);
    while (rootDir.exists(stagingBaseName))
        stagingBaseName = makeStagingDirName(packName + "_retry");

    const QString stagingDir = rootDir.filePath(stagingBaseName);
    if (!QDir().mkpath(stagingDir)) {
        result.errors.append(QObject::tr("failed to create staging directory '%1'").arg(stagingDir));
        return result;
    }

    QJsonObject pack;
    pack["name"] = spec.displayName.isEmpty() ? packName : spec.displayName;
    pack["version"] = spec.version;
    pack["author"] = spec.author;
    pack["description"] = spec.description;
    pack["kind"] = "imported_library_pack";
    pack["language"] = spec.language;
    pack["standard"] = spec.standard;
    pack["category"] = sanitizeSegment(spec.category);
    pack["source_headers"] = toJsonArray(spec.headerPaths);
    pack["include_paths"] = toJsonArray(spec.includePaths);
    pack["defines"] = toJsonArray(spec.defines);
    pack["link_libraries"] = toJsonArray(spec.linkLibraries);

    QFile packFile(stagingDir + "/pack.json");
    if (!packFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errors.append(QObject::tr("failed to write pack.json for '%1'").arg(packName));
        QDir(stagingDir).removeRecursively();
        return result;
    }
    packFile.write(QJsonDocument(pack).toJson(QJsonDocument::Indented));
    packFile.close();

    for (const auto &module : modules) {
        Module prepared = prepareModuleForImportedPack(module, spec, &result.warnings);

        const QString categoryDir = QDir(stagingDir).filePath(
            prepared.category.isEmpty() ? QString("imported") : prepared.category);
        if (!QDir().mkpath(categoryDir)) {
            result.errors.append(QObject::tr("failed to create category dir '%1'").arg(categoryDir));
            QDir(stagingDir).removeRecursively();
            return result;
        }

        const QString moduleFilePath = QDir(categoryDir).filePath(sanitizeSegment(prepared.name) + ".dqmod");
        QFile moduleFile(moduleFilePath);
        if (!moduleFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            result.errors.append(QObject::tr("failed to write module file '%1'").arg(moduleFilePath));
            QDir(stagingDir).removeRecursively();
            return result;
        }
        moduleFile.write(QJsonDocument(prepared.toJson()).toJson(QJsonDocument::Indented));
        moduleFile.close();
        result.writtenModuleFiles.append(moduleFilePath);
    }

    if (!wrappers.isEmpty()) {
        const QString wrappersDir = QDir(stagingDir).filePath("wrappers");
        if (!QDir().mkpath(wrappersDir)) {
            result.errors.append(QObject::tr("failed to create wrappers dir '%1'").arg(wrappersDir));
            QDir(stagingDir).removeRecursively();
            return result;
        }

        for (const auto &wrapper : wrappers) {
            const QString baseName = sanitizeSegment(wrapper.className);
            const QString headerPath = QDir(wrappersDir).filePath(baseName + ".h");
            const QString sourcePath = QDir(wrappersDir).filePath(baseName + ".cpp");

            QFile headerFile(headerPath);
            if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result.errors.append(QObject::tr("failed to write wrapper header '%1'").arg(headerPath));
                QDir(stagingDir).removeRecursively();
                return result;
            }
            headerFile.write(wrapper.header.toUtf8());
            headerFile.close();

            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result.errors.append(QObject::tr("failed to write wrapper source '%1'").arg(sourcePath));
                QDir(stagingDir).removeRecursively();
                return result;
            }
            sourceFile.write(wrapper.source.toUtf8());
            sourceFile.close();

            result.writtenWrapperFiles.append(headerPath);
            result.writtenWrapperFiles.append(sourcePath);
        }
    }

    QString backupBaseName;
    if (rootDir.exists(packName)) {
        backupBaseName = QString(".dq_import_backup_%1_%2")
            .arg(packName)
            .arg(QString::number(QDateTime::currentMSecsSinceEpoch()));
        while (rootDir.exists(backupBaseName))
            backupBaseName.append("_retry");

        if (!rootDir.rename(packName, backupBaseName)) {
            result.errors.append(QObject::tr("failed to prepare existing pack '%1' for replacement").arg(packDir));
            QDir(stagingDir).removeRecursively();
            return result;
        }
    }

    if (!rootDir.rename(stagingBaseName, packName)) {
        if (!backupBaseName.isEmpty())
            rootDir.rename(backupBaseName, packName);
        result.errors.append(QObject::tr("failed to activate imported pack '%1'").arg(packDir));
        QDir(stagingDir).removeRecursively();
        return result;
    }

    if (!backupBaseName.isEmpty())
        QDir(rootDir.filePath(backupBaseName)).removeRecursively();

    for (QString &writtenModuleFile : result.writtenModuleFiles) {
        const QString relativePath = QDir(stagingDir).relativeFilePath(writtenModuleFile);
        writtenModuleFile = QDir(packDir).filePath(relativePath);
    }
    for (QString &writtenWrapperFile : result.writtenWrapperFiles) {
        const QString relativePath = QDir(stagingDir).relativeFilePath(writtenWrapperFile);
        writtenWrapperFile = QDir(packDir).filePath(relativePath);
    }

    return result;
}

} // namespace DeltaQ

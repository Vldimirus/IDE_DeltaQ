// Упаковка импортированной библиотеки в extension pack DeltaQ — реализация
#include "LibraryPackager.h"

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

} // namespace

QString LibraryPackager::sanitizePackName(const QString &name)
{
    return sanitizeSegment(name);
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
    if (packName.isEmpty()) {
        result.errors.append(QObject::tr("pack name is empty"));
        return result;
    }

    const QString packDir = QDir(modulesRootDir).filePath(packName);
    result.packDir = packDir;

    if (!QDir().mkpath(packDir)) {
        result.errors.append(QObject::tr("failed to create pack directory '%1'").arg(packDir));
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

    QFile packFile(packDir + "/pack.json");
    if (!packFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errors.append(QObject::tr("failed to write pack.json for '%1'").arg(packName));
        return result;
    }
    packFile.write(QJsonDocument(pack).toJson(QJsonDocument::Indented));
    packFile.close();

    for (const auto &module : modules) {
        Module prepared = prepareModuleForImportedPack(module, spec, &result.warnings);

        const QString categoryDir = QDir(packDir).filePath(
            prepared.category.isEmpty() ? QString("imported") : prepared.category);
        if (!QDir().mkpath(categoryDir)) {
            result.errors.append(QObject::tr("failed to create category dir '%1'").arg(categoryDir));
            return result;
        }

        const QString moduleFilePath = QDir(categoryDir).filePath(sanitizeSegment(prepared.name) + ".dqmod");
        QFile moduleFile(moduleFilePath);
        if (!moduleFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            result.errors.append(QObject::tr("failed to write module file '%1'").arg(moduleFilePath));
            return result;
        }
        moduleFile.write(QJsonDocument(prepared.toJson()).toJson(QJsonDocument::Indented));
        moduleFile.close();
        result.writtenModuleFiles.append(moduleFilePath);
    }

    if (!wrappers.isEmpty()) {
        const QString wrappersDir = QDir(packDir).filePath("wrappers");
        if (!QDir().mkpath(wrappersDir)) {
            result.errors.append(QObject::tr("failed to create wrappers dir '%1'").arg(wrappersDir));
            return result;
        }

        for (const auto &wrapper : wrappers) {
            const QString baseName = sanitizeSegment(wrapper.className);
            const QString headerPath = QDir(wrappersDir).filePath(baseName + ".h");
            const QString sourcePath = QDir(wrappersDir).filePath(baseName + ".cpp");

            QFile headerFile(headerPath);
            if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result.errors.append(QObject::tr("failed to write wrapper header '%1'").arg(headerPath));
                return result;
            }
            headerFile.write(wrapper.header.toUtf8());
            headerFile.close();

            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                result.errors.append(QObject::tr("failed to write wrapper source '%1'").arg(sourcePath));
                return result;
            }
            sourceFile.write(wrapper.source.toUtf8());
            sourceFile.close();

            result.writtenWrapperFiles.append(headerPath);
            result.writtenWrapperFiles.append(sourcePath);
        }
    }

    return result;
}

} // namespace DeltaQ

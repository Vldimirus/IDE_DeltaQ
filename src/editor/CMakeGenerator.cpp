// Генератор CMakeLists.txt для пользовательских проектов DeltaQ
#include "CMakeGenerator.h"
#include "../core/GraphStore.h"
#include "../core/ModuleRegistry.h"

#include <deltaq/Graph.h>
#include <deltaq/Module.h>

#include <algorithm>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QTextStream>
#include <QProcess>

namespace DeltaQ {

namespace {

// Забирает строковый список из metadata-массива модуля без дублирования кода.
QStringList metadataStringList(const Module &module, const QString &key)
{
    QStringList result;
    const QJsonArray values = module.metadata.value(key).toArray();
    for (const auto &value : values) {
        const QString text = value.toString().trimmed();
        if (!text.isEmpty())
            result.append(text);
    }
    return result;
}

// Добавляет строки в список только один раз и сохраняет стабильный порядок.
void appendUnique(QStringList &target, const QStringList &values)
{
    for (const auto &value : values) {
        if (!target.contains(value))
            target.append(value);
    }
}

// Форматирует элемент списка для CMake так, чтобы пути со пробелами не ломали файл.
QString formatCMakeValue(const QString &value)
{
    const QString normalized = QDir::fromNativeSeparators(value);
    QString escaped = normalized;
    escaped.replace("\\", "/");
    escaped.replace("\"", "\\\"");
    if (escaped.contains(' ') || escaped.contains(';'))
        return QString("\"%1\"").arg(escaped);
    return escaped;
}

} // namespace

CMakeGenerator::CMakeGenerator(QObject *parent)
    : QObject(parent)
{
}

QString CMakeGenerator::generate(const QString &projectDir, const QString &projectName,
                                  const QString &cStandard, const QString &cxxStandard,
                                  const QStringList &extraFlags, const QString &projectType)
{
    QStringList sources = collectSources(projectDir);
    const QVector<ImportedPackRequirement> importedPackRequirements = collectImportedPackRequirements();
    QString content = generateContent(projectName, sources, cStandard, cxxStandard,
                                      extraFlags, projectType, importedPackRequirements);

    // Записываем CMakeLists.txt в корень проекта
    QString cmakePath = projectDir + "/CMakeLists.txt";
    QFile existingFile(cmakePath);
    if (existingFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString existingContent = QString::fromUtf8(existingFile.readAll());
        existingFile.close();
        if (existingContent == content)
            return cmakePath;
    }

    QFile file(cmakePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << content;
        file.close();
    }
    return cmakePath;
}

bool CMakeGenerator::configure(const QString &projectDir)
{
    QString buildDir = projectDir + "/build";
    QDir().mkpath(buildDir);

    QProcess cmake;
    cmake.setWorkingDirectory(buildDir);
    cmake.start("cmake", {".."});
    return cmake.waitForFinished(30000) && cmake.exitCode() == 0;
}

QStringList CMakeGenerator::collectSources(const QString &projectDir) const
{
    QStringList sources;
    QStringList filters = {"*.c", "*.cpp", "*.cxx", "*.cc"};

    QDirIterator it(projectDir, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString relPath = QDir(projectDir).relativeFilePath(it.filePath());
        // Пропускаем build/ директорию
        if (relPath.startsWith("build/") || relPath.startsWith("build\\"))
            continue;
        sources.append(relPath);
    }

    // Файлы из src/ уже подхватываются рекурсивным итератором

    sources.sort();
    return sources;
}

QVector<CMakeGenerator::ImportedPackRequirement> CMakeGenerator::collectImportedPackRequirements() const
{
    QVector<ImportedPackRequirement> result;
    if (!m_moduleRegistry || !m_graphStore)
        return result;

    QMap<QString, ImportedPackRequirement> requirements;
    QSet<QString> visitedCompositeModules;

    // Анализируем только корневые графы: именно из них pre-build генерирует translation units проекта.
    for (const Graph *graph : m_graphStore->allGraphs()) {
        if (!graph || !graph->parentModuleId.isEmpty())
            continue;
        collectImportedPackRequirementsFromGraph(*graph, requirements, visitedCompositeModules);
    }

    result = requirements.values().toVector();
    std::sort(result.begin(), result.end(),
              [](const ImportedPackRequirement &lhs, const ImportedPackRequirement &rhs) {
        return lhs.packName < rhs.packName;
    });
    return result;
}

void CMakeGenerator::collectImportedPackRequirementsFromGraph(
    const Graph &graph,
    QMap<QString, ImportedPackRequirement> &requirements,
    QSet<QString> &visitedCompositeModules) const
{
    for (const auto &node : graph.nodes) {
        const Module *module = m_moduleRegistry->findModule(node.moduleId);
        if (!module)
            continue;
        collectImportedPackRequirementsFromModule(*module, requirements, visitedCompositeModules);
    }
}

void CMakeGenerator::collectImportedPackRequirementsFromModule(
    const Module &module,
    QMap<QString, ImportedPackRequirement> &requirements,
    QSet<QString> &visitedCompositeModules) const
{
    const QString packName = module.metadataString("deltaq.import.pack_name");
    if (!packName.isEmpty()) {
        ImportedPackRequirement &requirement = requirements[packName];
        requirement.packName = packName;
        appendUnique(requirement.includePaths,
                     metadataStringList(module, "deltaq.import.include_paths"));
        appendUnique(requirement.defines,
                     metadataStringList(module, "deltaq.import.defines"));
        appendUnique(requirement.linkLibraries,
                     metadataStringList(module, "deltaq.import.link_libraries"));
    }

    // Для составных модулей спускаемся во внутренний граф, чтобы CMake увидел требования
    // imported pack-ов, скрытых внутри reusable submodule.
    if (!module.isComposite() || !m_graphStore || visitedCompositeModules.contains(module.id))
        return;

    visitedCompositeModules.insert(module.id);
    const Graph *innerGraph = m_graphStore->findGraph(module.graphId);
    if (innerGraph)
        collectImportedPackRequirementsFromGraph(*innerGraph, requirements, visitedCompositeModules);
}

QString CMakeGenerator::generateContent(const QString &projectName,
                                         const QStringList &sources,
                                         const QString &cStandard,
                                         const QString &cxxStandard,
                                         const QStringList &extraFlags,
                                         const QString &projectType,
                                         const QVector<ImportedPackRequirement> &importedPackRequirements) const
{
    QString cmake;
    cmake += "# Автоматически сгенерировано DeltaQ IDE\n";
    cmake += "cmake_minimum_required(VERSION 3.20)\n\n";
    cmake += QString("project(%1 LANGUAGES C CXX)\n\n").arg(projectName);

    // Стандарты
    cmake += QString("set(CMAKE_C_STANDARD %1)\n").arg(cStandard);
    cmake += "set(CMAKE_C_STANDARD_REQUIRED ON)\n";
    cmake += QString("set(CMAKE_CXX_STANDARD %1)\n").arg(cxxStandard);
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";

    // Исходные файлы
    cmake += "# Исходные файлы\n";
    cmake += "set(SOURCES\n";
    for (const auto &src : sources)
        cmake += QString("    %1\n").arg(src);
    cmake += ")\n\n";

    // Исполняемый файл
    cmake += QString("add_executable(%1 ${SOURCES})\n\n").arg(projectName);

    // Include директории
    cmake += QString("target_include_directories(%1 PRIVATE\n").arg(projectName);
    cmake += "    ${CMAKE_SOURCE_DIR}\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/include\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/src\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/src/ui\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/ui\n";
    cmake += ")\n\n";

    if (!importedPackRequirements.isEmpty()) {
        cmake += "# Imported DeltaQ module packs actually used by project graphs\n";
        for (const auto &requirement : importedPackRequirements)
            cmake += QString("#   - %1\n").arg(requirement.packName);
        cmake += "\n";

        QStringList importedIncludePaths;
        QStringList importedDefines;
        QStringList importedLinkLibraries;
        for (const auto &requirement : importedPackRequirements) {
            appendUnique(importedIncludePaths, requirement.includePaths);
            appendUnique(importedDefines, requirement.defines);
            appendUnique(importedLinkLibraries, requirement.linkLibraries);
        }

        if (!importedIncludePaths.isEmpty()) {
            cmake += QString("target_include_directories(%1 PRIVATE\n").arg(projectName);
            for (const auto &path : importedIncludePaths)
                cmake += QString("    %1\n").arg(formatCMakeValue(path));
            cmake += ")\n\n";
        }

        if (!importedDefines.isEmpty()) {
            cmake += QString("target_compile_definitions(%1 PRIVATE\n").arg(projectName);
            for (const auto &define : importedDefines)
                cmake += QString("    %1\n").arg(define);
            cmake += ")\n\n";
        }

        if (!importedLinkLibraries.isEmpty()) {
            cmake += QString("target_link_libraries(%1 PRIVATE\n").arg(projectName);
            for (const auto &library : importedLinkLibraries)
                cmake += QString("    %1\n").arg(formatCMakeValue(library));
            cmake += ")\n\n";
        }
    }

    // Флаги компиляции
    cmake += QString("target_compile_options(%1 PRIVATE\n").arg(projectName);
    cmake += "    -Wall -Wextra\n";
    for (const auto &flag : extraFlags)
        cmake += QString("    %1\n").arg(flag);
    cmake += ")\n";

    // SDL2 для desktop-проектов (CMake package -> pkg-config fallback)
    if (projectType == "desktop") {
        cmake += "\n# SDL2 / SDL2_ttf\n";
        cmake += "find_package(SDL2 QUIET)\n";
        cmake += "find_package(SDL2_ttf QUIET)\n\n";
        cmake += "if (TARGET SDL2::SDL2 AND TARGET SDL2_ttf::SDL2_ttf)\n";
        cmake += QString("    target_link_libraries(%1 PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)\n").arg(projectName);
        cmake += "else()\n";
        cmake += "    find_package(PkgConfig QUIET)\n";
        cmake += "    if (PkgConfig_FOUND)\n";
        cmake += "        pkg_check_modules(PKG_SDL2 QUIET sdl2)\n";
        cmake += "        pkg_check_modules(PKG_SDL2_TTF QUIET SDL2_ttf)\n";
        cmake += "    endif()\n\n";
        cmake += "    if (PKG_SDL2_FOUND AND PKG_SDL2_TTF_FOUND)\n";
        cmake += QString("        target_include_directories(%1 PRIVATE ${PKG_SDL2_INCLUDE_DIRS} ${PKG_SDL2_TTF_INCLUDE_DIRS})\n").arg(projectName);
        cmake += QString("        target_link_directories(%1 PRIVATE ${PKG_SDL2_LIBRARY_DIRS} ${PKG_SDL2_TTF_LIBRARY_DIRS})\n").arg(projectName);
        cmake += QString("        target_link_libraries(%1 PRIVATE ${PKG_SDL2_LIBRARIES} ${PKG_SDL2_TTF_LIBRARIES})\n").arg(projectName);
        cmake += "    else()\n";
        cmake += "        message(FATAL_ERROR\n";
        cmake += "            \"Desktop template requires SDL2 and SDL2_ttf.\\n\"\n";
        cmake += "            \"Install dependencies and re-run configure:\\n\"\n";
        cmake += "            \"  Debian/Ubuntu: sudo apt install libsdl2-dev libsdl2-ttf-dev\\n\"\n";
        cmake += "            \"  Fedora: sudo dnf install SDL2-devel SDL2_ttf-devel\\n\"\n";
        cmake += "            \"  Arch: sudo pacman -S sdl2 sdl2_ttf\")\n";
        cmake += "    endif()\n";
        cmake += "endif()\n";
    }

    return cmake;
}

} // namespace DeltaQ

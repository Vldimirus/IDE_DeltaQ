// Генератор CMakeLists.txt для пользовательских проектов DeltaQ
#include "CMakeGenerator.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QProcess>

namespace DeltaQ {

CMakeGenerator::CMakeGenerator(QObject *parent)
    : QObject(parent)
{
}

QString CMakeGenerator::generate(const QString &projectDir, const QString &projectName,
                                  const QString &cStandard, const QString &cxxStandard,
                                  const QStringList &extraFlags, const QString &projectType)
{
    QStringList sources = collectSources(projectDir);
    QString content = generateContent(projectName, sources, cStandard, cxxStandard, extraFlags, projectType);

    // Записываем CMakeLists.txt в корень проекта
    QString cmakePath = projectDir + "/CMakeLists.txt";
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

    // Добавляем сгенерированные файлы из generated/
    QDir genDir(projectDir + "/generated");
    if (genDir.exists()) {
        QDirIterator genIt(genDir.absolutePath(), filters, QDir::Files);
        while (genIt.hasNext()) {
            genIt.next();
            QString relPath = QDir(projectDir).relativeFilePath(genIt.filePath());
            if (!sources.contains(relPath))
                sources.append(relPath);
        }
    }

    sources.sort();
    return sources;
}

QString CMakeGenerator::generateContent(const QString &projectName,
                                         const QStringList &sources,
                                         const QString &cStandard,
                                         const QString &cxxStandard,
                                         const QStringList &extraFlags,
                                         const QString &projectType) const
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
    cmake += "    ${CMAKE_SOURCE_DIR}/ui\n";
    cmake += ")\n\n";

    // Флаги компиляции
    cmake += QString("target_compile_options(%1 PRIVATE\n").arg(projectName);
    cmake += "    -Wall -Wextra\n";
    for (const auto &flag : extraFlags)
        cmake += QString("    %1\n").arg(flag);
    cmake += ")\n";

    // SDL2 для desktop-проектов
    if (projectType == "desktop") {
        cmake += "\n# SDL2\n";
        cmake += "find_package(PkgConfig REQUIRED)\n";
        cmake += "pkg_check_modules(SDL2 REQUIRED sdl2)\n";
        cmake += QString("target_include_directories(%1 PRIVATE ${SDL2_INCLUDE_DIRS})\n").arg(projectName);
        cmake += QString("target_link_libraries(%1 PRIVATE ${SDL2_LIBRARIES})\n").arg(projectName);
    }

    return cmake;
}

} // namespace DeltaQ

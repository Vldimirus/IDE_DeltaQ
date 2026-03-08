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

    // Файлы из src/ уже подхватываются рекурсивным итератором

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
    cmake += "    ${CMAKE_SOURCE_DIR}/src\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/src/ui\n";
    cmake += "    ${CMAKE_SOURCE_DIR}/ui\n";
    cmake += ")\n\n";

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

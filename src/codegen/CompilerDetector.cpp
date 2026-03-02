// Автоопределение компилятора — реализация
#include "CompilerDetector.h"

#include <QProcess>

namespace DeltaQ {

CompilerDetector::CompilerDetector(QObject *parent)
    : QObject(parent)
{
}

void CompilerDetector::detect()
{
    m_compilers.clear();
    m_buildTools.clear();

    // Проверяем C-компиляторы
    for (const auto &name : {"gcc", "clang", "cc"})
        m_compilers.append(probeCompiler(name));

    // Проверяем C++-компиляторы
    for (const auto &name : {"g++", "clang++", "c++"})
        m_compilers.append(probeCompiler(name));

    // Проверяем инструменты сборки
    for (const auto &name : {"cmake", "make", "ninja"})
        m_buildTools.append(probeBuildTool(name));

    emit detectionComplete();
}

CompilerInfo CompilerDetector::bestCCompiler() const
{
    // Приоритет: gcc > clang > cc
    QStringList priority = {"gcc", "clang", "cc"};
    for (const auto &name : priority) {
        for (const auto &c : m_compilers) {
            if (c.name == name && c.available)
                return c;
        }
    }
    return {};
}

CompilerInfo CompilerDetector::bestCppCompiler() const
{
    QStringList priority = {"g++", "clang++", "c++"};
    for (const auto &name : priority) {
        for (const auto &c : m_compilers) {
            if (c.name == name && c.available)
                return c;
        }
    }
    return {};
}

bool CompilerDetector::hasCMake() const
{
    for (const auto &t : m_buildTools)
        if (t.name == "cmake" && t.available) return true;
    return false;
}

bool CompilerDetector::hasMake() const
{
    for (const auto &t : m_buildTools)
        if (t.name == "make" && t.available) return true;
    return false;
}

bool CompilerDetector::hasNinja() const
{
    for (const auto &t : m_buildTools)
        if (t.name == "ninja" && t.available) return true;
    return false;
}

QString CompilerDetector::bestCompilerPath() const
{
    auto cc = bestCCompiler();
    if (cc.available)
        return cc.path;
    return {};
}

CompilerInfo CompilerDetector::probeCompiler(const QString &name)
{
    CompilerInfo info;
    info.name = name;

    QProcess proc;
    proc.start(name, {"--version"});
    if (!proc.waitForFinished(3000)) {
        info.available = false;
        return info;
    }

    if (proc.exitCode() != 0) {
        info.available = false;
        return info;
    }

    info.available = true;

    // Извлекаем версию из первой строки вывода
    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    if (!lines.isEmpty())
        info.version = lines.first().trimmed();

    // Определяем полный путь через which/where
    QProcess which;
    which.start("which", {name});
    if (which.waitForFinished(2000) && which.exitCode() == 0) {
        info.path = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
    } else {
        info.path = name; // fallback: используем имя как путь
    }

    return info;
}

BuildToolInfo CompilerDetector::probeBuildTool(const QString &name)
{
    BuildToolInfo info;
    info.name = name;

    QProcess proc;
    proc.start(name, {"--version"});
    if (!proc.waitForFinished(3000)) {
        info.available = false;
        return info;
    }

    if (proc.exitCode() != 0) {
        info.available = false;
        return info;
    }

    info.available = true;

    QString output = proc.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    if (!lines.isEmpty())
        info.version = lines.first().trimmed();

    QProcess which;
    which.start("which", {name});
    if (which.waitForFinished(2000) && which.exitCode() == 0)
        info.path = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
    else
        info.path = name;

    return info;
}

} // namespace DeltaQ

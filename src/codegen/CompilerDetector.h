// Автоопределение компилятора и инструментов сборки
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace DeltaQ {

struct CompilerInfo {
    QString name;       // "gcc", "clang", "g++", "clang++"
    QString path;       // Полный путь к исполняемому файлу
    QString version;    // Версия (из --version)
    bool available = false;
};

struct BuildToolInfo {
    QString name;       // "cmake", "make", "ninja"
    QString path;
    QString version;
    bool available = false;
};

class CompilerDetector : public QObject {
    Q_OBJECT

public:
    explicit CompilerDetector(QObject *parent = nullptr);

    // Обнаружение доступных компиляторов и инструментов
    void detect();

    // Результаты обнаружения
    QVector<CompilerInfo> compilers() const { return m_compilers; }
    QVector<BuildToolInfo> buildTools() const { return m_buildTools; }

    // Лучший доступный C-компилятор (gcc > clang > cc)
    CompilerInfo bestCCompiler() const;

    // Лучший доступный C++-компилятор (g++ > clang++ > c++)
    CompilerInfo bestCppCompiler() const;

    // Проверка наличия инструментов сборки
    bool hasCMake() const;
    bool hasMake() const;
    bool hasNinja() const;

    // Полный путь к лучшему компилятору (для BuildConfig::compilerPath)
    QString bestCompilerPath() const;

signals:
    void detectionComplete();

private:
    CompilerInfo probeCompiler(const QString &name);
    BuildToolInfo probeBuildTool(const QString &name);

    QVector<CompilerInfo> m_compilers;
    QVector<BuildToolInfo> m_buildTools;
};

} // namespace DeltaQ

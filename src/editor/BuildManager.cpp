// Менеджер сборки — интеграция с CMakeGenerator и CompilerOutputParser
#include "BuildManager.h"
#include "CompilerOutputParser.h"
#include "CMakeGenerator.h"
#include <QDir>
#include <QFile>

namespace DeltaQ {

BuildManager::BuildManager(QObject *parent)
    : QObject(parent)
    , m_parser(new CompilerOutputParser(this))
    , m_generator(new CMakeGenerator(this))
{
    // Пробрасываем ошибки компилятора как сигналы
    connect(m_parser, &CompilerOutputParser::errorFound, this, [this](const CompilerError &err) {
        QString sev = err.isError() ? "error" : (err.isWarning() ? "warning" : "note");
        emit buildError(err.file, err.line, err.column, sev, err.message);
    });
}

void BuildManager::build(const QString &projectDir, const QString &projectName,
                          const QString &cStandard, const QString &cxxStandard,
                          const QString &projectType)
{
    if (isBuilding())
        return;

    m_parser->clear();

    QString buildDir = projectDir + "/build";
    QDir().mkpath(buildDir);
    m_currentBuildDir = buildDir;

    emit buildStarted();

    // Если есть имя проекта — генерируем CMakeLists.txt
    if (!projectName.isEmpty()) {
        emit buildOutput(tr("=== Generating CMakeLists.txt ===\n"));
        m_generator->generate(projectDir, projectName, cStandard, cxxStandard, {}, projectType);
    }

    // Проверяем есть ли CMakeLists.txt
    if (!QFile::exists(projectDir + "/CMakeLists.txt")) {
        emit buildOutput(tr("=== Error: CMakeLists.txt not found ===\n"));
        emit buildFinished(false, 1, 0);
        return;
    }

    // Проверяем нужна ли конфигурация (если нет CMakeCache.txt)
    if (!QFile::exists(buildDir + "/CMakeCache.txt")) {
        emit buildOutput(tr("=== Configuring with CMake ===\n"));

        m_process = new QProcess(this);
        m_process->setWorkingDirectory(buildDir);
        connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
        connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus status) {
            if (status == QProcess::NormalExit && exitCode == 0) {
                m_process->deleteLater();
                m_process = nullptr;
                // После успешной конфигурации — запускаем сборку
                runBuild(m_currentBuildDir);
            } else {
                emit buildOutput(tr("\n=== CMake configure failed ===\n"));
                emit buildFinished(false, m_parser->errorCount(), m_parser->warningCount());
                m_process->deleteLater();
                m_process = nullptr;
            }
        });
        m_process->start("cmake", {".."});
    } else {
        // CMake уже сконфигурирован — сразу собираем
        runBuild(buildDir);
    }
}

void BuildManager::runBuild(const QString &buildDir)
{
    emit buildOutput(tr("=== Building project ===\n"));

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(buildDir);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildManager::onProcessFinished);

    m_process->start("cmake", {"--build", ".", "--parallel"});
}

void BuildManager::clean(const QString &projectDir)
{
    QString buildDir = projectDir + "/build";
    QDir(buildDir).removeRecursively();
    emit buildOutput(tr("=== Clean complete ===\n"));
}

void BuildManager::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(2000))
            m_process->kill();
        emit buildOutput(tr("\n=== Build cancelled ===\n"));
        emit buildFinished(false, m_parser->errorCount(), m_parser->warningCount());
    }
}

void BuildManager::onProcessOutput()
{
    if (!m_process)
        return;
    QString out = m_process->readAllStandardOutput();
    QString err = m_process->readAllStandardError();

    if (!out.isEmpty()) {
        emit buildOutput(out);
        // Парсим на ошибки
        for (const auto &line : out.split('\n'))
            m_parser->parseLine(line);
    }
    if (!err.isEmpty()) {
        emit buildOutput(err);
        for (const auto &line : err.split('\n'))
            m_parser->parseLine(line);
    }
}

void BuildManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    bool success = (status == QProcess::NormalExit && exitCode == 0);
    int errors = m_parser->errorCount();
    int warnings = m_parser->warningCount();

    if (success)
        emit buildOutput(tr("\n=== Build succeeded ===\n"));
    else
        emit buildOutput(tr("\n=== Build failed (exit code: %1) ===\n").arg(exitCode));

    if (errors > 0 || warnings > 0)
        emit buildOutput(tr("    %1 error(s), %2 warning(s)\n").arg(errors).arg(warnings));

    emit buildFinished(success, errors, warnings);
    m_process->deleteLater();
    m_process = nullptr;
}

int BuildManager::lastErrorCount() const
{
    return m_parser->errorCount();
}

int BuildManager::lastWarningCount() const
{
    return m_parser->warningCount();
}

} // namespace DeltaQ

// Менеджер сборки — интеграция с CMakeGenerator и CompilerOutputParser
#include "BuildManager.h"
#include "CompilerOutputParser.h"
#include "CMakeGenerator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

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

    QString configureReason;
    if (shouldConfigure(projectDir, buildDir, &configureReason)) {
        const QString heading = configureReason.isEmpty()
            ? tr("=== Configuring with CMake ===\n")
            : tr("=== Re-configuring build directory ===\n");
        emit buildOutput(heading);
        if (!configureReason.isEmpty())
            emit buildOutput(tr("Reason: %1\n").arg(configureReason));

        m_process = new QProcess(this);
        m_process->setWorkingDirectory(buildDir);
        connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
        connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus status) {
            if (status == QProcess::NormalExit && exitCode == 0) {
                const QString buildArtifact = expectedBuildArtifact(m_currentBuildDir);
                if (buildArtifact.isEmpty() || !QFile::exists(buildArtifact)) {
                    emit buildOutput(tr("\n=== CMake configure failed ===\n"));
                    emit buildOutput(tr("Expected build artifact was not created: %1\n")
                                     .arg(buildArtifact.isEmpty() ? tr("<unknown>") : buildArtifact));
                    emit buildFinished(false, m_parser->errorCount(), m_parser->warningCount());
                    m_process->deleteLater();
                    m_process = nullptr;
                    return;
                }
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

bool BuildManager::shouldConfigure(const QString &projectDir, const QString &buildDir,
                                   QString *reason) const
{
    const QString cachePath = buildDir + "/CMakeCache.txt";
    if (!QFile::exists(cachePath)) {
        if (reason)
            reason->clear();
        return true;
    }

    const QString buildArtifact = expectedBuildArtifact(buildDir);
    if (buildArtifact.isEmpty() || !QFile::exists(buildArtifact)) {
        if (reason) {
            *reason = buildArtifact.isEmpty()
                ? tr("CMake generator is unknown; forcing a fresh configure")
                : tr("Missing generated build file: %1").arg(QDir(buildDir).relativeFilePath(buildArtifact));
        }
        return true;
    }

    QFileInfo cmakeInfo(projectDir + "/CMakeLists.txt");
    QFileInfo cacheInfo(cachePath);
    if (cmakeInfo.exists() && cacheInfo.exists() &&
        cmakeInfo.lastModified() > cacheInfo.lastModified()) {
        if (reason)
            *reason = tr("Project CMakeLists.txt is newer than the last configure");
        return true;
    }

    if (reason)
        reason->clear();
    return false;
}

QString BuildManager::cacheValue(const QString &cachePath, const QString &key) const
{
    QFile cacheFile(cachePath);
    if (!cacheFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&cacheFile);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith('#') || line.startsWith("//") || !line.contains('='))
            continue;

        const int equalsPos = line.indexOf('=');
        const QString lhs = line.left(equalsPos);
        const int colonPos = lhs.indexOf(':');
        const QString currentKey = (colonPos >= 0) ? lhs.left(colonPos) : lhs;
        if (currentKey == key)
            return line.mid(equalsPos + 1).trimmed();
    }

    return {};
}

QString BuildManager::expectedBuildArtifact(const QString &buildDir) const
{
    const QString generator = cacheValue(buildDir + "/CMakeCache.txt", "CMAKE_GENERATOR");
    if (generator == "Unix Makefiles")
        return buildDir + "/Makefile";
    if (generator == "Ninja" || generator == "Ninja Multi-Config")
        return buildDir + "/build.ninja";
    return {};
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

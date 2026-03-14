// Менеджер сборки — интеграция с CMakeGenerator и CompilerOutputParser
#include "BuildManager.h"
#include "CompilerOutputParser.h"
#include "CMakeGenerator.h"
#include <deltaq/BuildTypes.h>
#include "../core/GraphStore.h"
#include "../core/ModuleRegistry.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

namespace DeltaQ {

namespace {

QString normalizedPathForComparison(const QString &path)
{
    if (path.isEmpty())
        return {};

    const QFileInfo info(path);
    if (info.exists()) {
        const QString canonical = info.canonicalFilePath();
        if (!canonical.isEmpty())
            return canonical;
    }

    return QDir::cleanPath(path);
}

} // namespace

BuildManager::BuildManager(QObject *parent)
    : QObject(parent)
    , m_parser(new CompilerOutputParser(this))
    , m_generator(new CMakeGenerator(this))
{
    connect(m_parser, &CompilerOutputParser::errorFound, this, [this](const CompilerError &err) {
        const QString sev = err.isError() ? "error" : (err.isWarning() ? "warning" : "note");
        emit buildError(err.file, err.line, err.column, sev, err.message);
    });
}

void BuildManager::setModuleRegistry(ModuleRegistry *registry)
{
    m_generator->setModuleRegistry(registry);
}

void BuildManager::setGraphStore(GraphStore *store)
{
    m_generator->setGraphStore(store);
}

void BuildManager::build(const ProjectBuildRequest &request)
{
    if (isBuilding())
        return;

    m_parser->clear();
    m_currentRequest = request;

    const QString buildDir = request.projectDir + "/build";
    QDir().mkpath(buildDir);
    m_currentBuildDir = buildDir;

    emit buildStarted();

    if (!request.projectName.isEmpty()) {
        emit buildOutput(tr("=== Generating CMakeLists.txt ===\n"));
        m_generator->generate(request.projectDir,
                              request.projectName,
                              request.cStandard,
                              request.cxxStandard,
                              request.extraCFlags,
                              request.extraCxxFlags,
                              request.projectType);
    }

    if (!QFile::exists(request.projectDir + "/CMakeLists.txt")) {
        emit buildOutput(tr("=== Error: CMakeLists.txt not found ===\n"));
        emit buildFinished(false, 1, 0);
        return;
    }

    QString configureReason;
    if (shouldConfigure(request, &configureReason)) {
        emit buildOutput(configureReason.isEmpty()
                             ? tr("=== Configuring with CMake ===\n")
                             : tr("=== Re-configuring build directory ===\n"));
        if (!configureReason.isEmpty())
            emit buildOutput(tr("Reason: %1\n").arg(configureReason));
        emit buildOutput(tr("Generator: %1\n").arg(request.toolchain.generatorDisplayName));
        emit buildOutput(tr("Build profile: %1\n").arg(request.toolchain.buildProfile));
        emit buildOutput(tr("CMake: %1\n").arg(request.toolchain.cmakePath));
        emit buildOutput(tr("C compiler: %1\n").arg(request.toolchain.cCompilerPath));
        emit buildOutput(tr("C++ compiler: %1\n").arg(request.toolchain.cxxCompilerPath));

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

                writeConfiguredFingerprint(m_currentBuildDir, m_currentRequest.toolchain.fingerprint);

                m_process->deleteLater();
                m_process = nullptr;
                runBuild(m_currentRequest, m_currentBuildDir);
                return;
            }

            emit buildOutput(tr("\n=== CMake configure failed ===\n"));
            emit buildFinished(false, m_parser->errorCount(), m_parser->warningCount());
            m_process->deleteLater();
            m_process = nullptr;
        });
        m_process->start(request.toolchain.cmakePath, configureArguments(request));
        return;
    }

    runBuild(request, buildDir);
}

void BuildManager::runBuild(const ProjectBuildRequest &request, const QString &buildDir)
{
    emit buildOutput(tr("=== Building project ===\n"));

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(buildDir);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildManager::onProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildManager::onProcessOutput);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BuildManager::onProcessFinished);

    m_process->start(request.toolchain.cmakePath, {"--build", ".", "--parallel"});
}

bool BuildManager::shouldConfigure(const ProjectBuildRequest &request, QString *reason) const
{
    const QString buildDir = request.projectDir + "/build";
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

    if (!request.toolchain.fingerprint.isEmpty()) {
        const QString storedFingerprint = configuredFingerprint(buildDir);
        if (storedFingerprint.isEmpty()) {
            if (reason)
                *reason = tr("Missing DeltaQ configure fingerprint");
            return true;
        }
        if (storedFingerprint != request.toolchain.fingerprint) {
            if (reason)
                *reason = tr("Resolved toolchain fingerprint changed");
            return true;
        }
    }

    const QString configuredGenerator = cacheValue(cachePath, "CMAKE_GENERATOR");
    if (!request.toolchain.generatorDisplayName.isEmpty()
        && !configuredGenerator.isEmpty()
        && configuredGenerator != request.toolchain.generatorDisplayName) {
        if (reason)
            *reason = tr("Configured generator differs from selected generator");
        return true;
    }

    const QString configuredBuildType = cacheValue(cachePath, "CMAKE_BUILD_TYPE");
    if (!request.toolchain.buildProfile.isEmpty()
        && !configuredBuildType.isEmpty()
        && configuredBuildType != request.toolchain.buildProfile) {
        if (reason)
            *reason = tr("Configured build profile differs from selected profile");
        return true;
    }

    const QString configuredCCompiler = normalizedPathForComparison(
        cacheValue(cachePath, "CMAKE_C_COMPILER"));
    const QString configuredCxxCompiler = normalizedPathForComparison(
        cacheValue(cachePath, "CMAKE_CXX_COMPILER"));
    if (!request.toolchain.cCompilerPath.isEmpty()
        && !configuredCCompiler.isEmpty()
        && configuredCCompiler != normalizedPathForComparison(request.toolchain.cCompilerPath)) {
        if (reason)
            *reason = tr("Configured C compiler differs from selected compiler");
        return true;
    }
    if (!request.toolchain.cxxCompilerPath.isEmpty()
        && !configuredCxxCompiler.isEmpty()
        && configuredCxxCompiler != normalizedPathForComparison(request.toolchain.cxxCompilerPath)) {
        if (reason)
            *reason = tr("Configured C++ compiler differs from selected compiler");
        return true;
    }

    return shouldConfigure(request.projectDir, buildDir, reason);
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
    if (cmakeInfo.exists() && cacheInfo.exists()
        && cmakeInfo.lastModified() > cacheInfo.lastModified()) {
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

QString BuildManager::configuredFingerprintPath(const QString &buildDir) const
{
    return buildDir + "/.deltaq_configure_fingerprint";
}

QString BuildManager::configuredFingerprint(const QString &buildDir) const
{
    QFile file(configuredFingerprintPath(buildDir));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

QStringList BuildManager::configureArguments(const ProjectBuildRequest &request) const
{
    QStringList args{request.projectDir};
    if (!request.toolchain.generatorDisplayName.isEmpty())
        args << "-G" << request.toolchain.generatorDisplayName;
    if (!request.toolchain.buildProfile.isEmpty())
        args << QString("-DCMAKE_BUILD_TYPE=%1").arg(request.toolchain.buildProfile);
    if (!request.toolchain.cCompilerPath.isEmpty())
        args << QString("-DCMAKE_C_COMPILER=%1").arg(request.toolchain.cCompilerPath);
    if (!request.toolchain.cxxCompilerPath.isEmpty())
        args << QString("-DCMAKE_CXX_COMPILER=%1").arg(request.toolchain.cxxCompilerPath);
    if (!request.toolchain.builderPath.isEmpty())
        args << QString("-DCMAKE_MAKE_PROGRAM=%1").arg(request.toolchain.builderPath);
    return args;
}

void BuildManager::clean(const QString &projectDir)
{
    const QString buildDir = projectDir + "/build";
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
    const QString out = m_process->readAllStandardOutput();
    const QString err = m_process->readAllStandardError();

    if (!out.isEmpty()) {
        emit buildOutput(out);
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
    const bool success = (status == QProcess::NormalExit && exitCode == 0);
    const int errors = m_parser->errorCount();
    const int warnings = m_parser->warningCount();

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

bool BuildManager::writeConfiguredFingerprint(const QString &buildDir,
                                              const QString &fingerprint) const
{
    if (fingerprint.trimmed().isEmpty())
        return true;

    QFile file(configuredFingerprintPath(buildDir));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(fingerprint.toUtf8());
    return true;
}

} // namespace DeltaQ

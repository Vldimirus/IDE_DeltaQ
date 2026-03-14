// Менеджер сборки — запуск компиляции через CMake с парсингом ошибок
#pragma once

#include <deltaq/BuildTypes.h>

#include <QObject>
#include <QProcess>

namespace DeltaQ {

class CompilerOutputParser;
class CMakeGenerator;
class ModuleRegistry;
class GraphStore;
struct CompilerError;

class BuildManager : public QObject {
    Q_OBJECT

public:
    explicit BuildManager(QObject *parent = nullptr);

    void build(const ProjectBuildRequest &request);
    void clean(const QString &projectDir);
    void cancel();
    void setModuleRegistry(ModuleRegistry *registry);
    void setGraphStore(GraphStore *store);

    bool isBuilding() const { return m_process && m_process->state() != QProcess::NotRunning; }

    // Результаты последней сборки
    int lastErrorCount() const;
    int lastWarningCount() const;
    CompilerOutputParser *outputParser() const { return m_parser; }

    // Диагностика состояния build-директории
    bool shouldConfigure(const ProjectBuildRequest &request,
                         QString *reason = nullptr) const;
    bool shouldConfigure(const QString &projectDir, const QString &buildDir,
                         QString *reason = nullptr) const;
    QString cacheValue(const QString &cachePath, const QString &key) const;
    QString expectedBuildArtifact(const QString &buildDir) const;
    QString configuredFingerprintPath(const QString &buildDir) const;
    QString configuredFingerprint(const QString &buildDir) const;
    QStringList configureArguments(const ProjectBuildRequest &request) const;

signals:
    void buildStarted();
    void buildOutput(const QString &text);
    void buildFinished(bool success, int errors, int warnings);
    void buildError(const QString &file, int line, int column,
                    const QString &severity, const QString &message);

private slots:
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void runBuild(const ProjectBuildRequest &request, const QString &buildDir);
    bool writeConfiguredFingerprint(const QString &buildDir,
                                    const QString &fingerprint) const;

    QProcess *m_process = nullptr;
    CompilerOutputParser *m_parser = nullptr;
    CMakeGenerator *m_generator = nullptr;
    QString m_currentBuildDir;
    ProjectBuildRequest m_currentRequest;
};

} // namespace DeltaQ

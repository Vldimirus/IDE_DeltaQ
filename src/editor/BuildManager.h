// Менеджер сборки — запуск компиляции через CMake с парсингом ошибок
#pragma once

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

    void build(const QString &projectDir,
               const QString &projectName = {},
               const QString &cStandard = "17",
               const QString &cxxStandard = "20",
               const QString &projectType = "console");
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
    bool shouldConfigure(const QString &projectDir, const QString &buildDir,
                         QString *reason = nullptr) const;
    QString cacheValue(const QString &cachePath, const QString &key) const;
    QString expectedBuildArtifact(const QString &buildDir) const;

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
    void runBuild(const QString &buildDir);

    QProcess *m_process = nullptr;
    CompilerOutputParser *m_parser = nullptr;
    CMakeGenerator *m_generator = nullptr;
    QString m_currentBuildDir;
};

} // namespace DeltaQ

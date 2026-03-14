// Генератор CMakeLists.txt для пользовательских проектов DeltaQ
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QVector>

namespace DeltaQ {

struct Project;
struct BuildConfig;
struct Module;
struct Graph;
class ModuleRegistry;
class GraphStore;

class CMakeGenerator : public QObject {
    Q_OBJECT

public:
    explicit CMakeGenerator(QObject *parent = nullptr);

    // Генерация CMakeLists.txt на основе проекта
    // Возвращает путь к сгенерированному файлу
    QString generate(const QString &projectDir, const QString &projectName,
                     const QString &cStandard = "17",
                     const QString &cxxStandard = "20",
                     const QStringList &extraFlags = {},
                     const QString &projectType = "console");

    QString generate(const QString &projectDir, const QString &projectName,
                     const QString &cStandard,
                     const QString &cxxStandard,
                     const QStringList &extraCFlags,
                     const QStringList &extraCxxFlags,
                     const QString &projectType);

    // Привязывает реестр модулей, чтобы CMake мог учитывать реально используемые imported pack-ы.
    void setModuleRegistry(ModuleRegistry *registry) { m_moduleRegistry = registry; }

    // Привязывает хранилище графов, чтобы CMake анализировал только задействованные графы проекта.
    void setGraphStore(GraphStore *store) { m_graphStore = store; }

    // Запуск cmake configure
    bool configure(const QString &projectDir);

private:
    struct ImportedPackRequirement {
        QString packName;
        QStringList includePaths;
        QStringList defines;
        QStringList linkLibraries;
    };

    // Сбор исходных файлов из директории проекта
    QStringList collectSources(const QString &projectDir) const;

    // Собирает build-требования только от тех imported pack-ов, которые реально используются графами.
    QVector<ImportedPackRequirement> collectImportedPackRequirements() const;

    // Рекурсивно обходит корневой или внутренний граф и собирает зависимости imported pack-ов.
    void collectImportedPackRequirementsFromGraph(const Graph &graph,
                                                 QMap<QString, ImportedPackRequirement> &requirements,
                                                 QSet<QString> &visitedCompositeModules) const;

    // Для составных модулей спускается во внутренний граф, для imported-модулей забирает metadata.
    void collectImportedPackRequirementsFromModule(const Module &module,
                                                  QMap<QString, ImportedPackRequirement> &requirements,
                                                  QSet<QString> &visitedCompositeModules) const;

    // Генерация текста CMakeLists.txt
    QString generateContent(const QString &projectName,
                            const QStringList &sources,
                            const QString &cStandard,
                            const QString &cxxStandard,
                            const QStringList &extraCFlags,
                            const QStringList &extraCxxFlags,
                            const QString &projectType,
                            const QVector<ImportedPackRequirement> &importedPackRequirements) const;

    ModuleRegistry *m_moduleRegistry = nullptr;
    GraphStore *m_graphStore = nullptr;
};

} // namespace DeltaQ

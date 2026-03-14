// Экспорт уже собранного Linux-проекта в handoff-ready directory bundle.
#pragma once

#include <QObject>
#include <QString>

namespace DeltaQ {

struct LinuxExportOptions {
    QString projectDir;
    QString projectName;
    QString projectType = "console";
    QString buildDir;
    QString distRootDir;
    QString executablePath;
    bool cleanOutput = true;
    bool packageArchive = false;
    QString archiveBaseName;
};

struct LinuxExportResult {
    bool success = false;
    QString bundleDir;
    QString launcherPath;
    QString sourceExecutablePath;
    QString exportedExecutablePath;
    QString manifestPath;
    QString archivePath;
    QString errorMessage;
};

class LinuxProjectExporter : public QObject {
    Q_OBJECT

public:
    explicit LinuxProjectExporter(QObject *parent = nullptr);

    // Первый рабочий цикл закрывает console и desktop bundle через directory export.
    LinuxExportResult exportBuiltProject(const LinuxExportOptions &options) const;
};

} // namespace DeltaQ

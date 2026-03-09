// Файловые шаблоны проектов — discovery, чтение манифестов и копирование в проект
#include "ProjectTemplates.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace DeltaQ {

namespace {

constexpr auto ProjectNameToken = "__PROJECT_NAME__";

QString applyProjectNameToken(QString value, const QString &projectName)
{
    value.replace(ProjectNameToken, projectName);
    return value;
}

bool isTemplateTextFile(const QString &path)
{
    static const QStringList textExtensions = {
        "c", "cc", "cpp", "cxx", "h", "hpp",
        "txt", "md", "json", "cmake", "ini", "cfg", "xml",
        "dqproj", "dqgraph", "dqui"
    };

    const QFileInfo info(path);
    const QString suffix = info.suffix().toLower();
    return info.fileName() == "CMakeLists.txt" || textExtensions.contains(suffix);
}

QStringList collectTemplateSummaryFiles(const QString &templateDir)
{
    QStringList files;
    QDir root(templateDir);
    QDirIterator it(templateDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString relPath = root.relativeFilePath(path);
        if (relPath == "template.json")
            continue;
        files.append(relPath);
    }
    files.sort();
    return files;
}

QString discoverTemplatesDirInternal()
{
    QStringList startPoints;
    if (!QCoreApplication::applicationDirPath().isEmpty())
        startPoints.append(QCoreApplication::applicationDirPath());
    if (!QDir::currentPath().isEmpty())
        startPoints.append(QDir::currentPath());

    for (const QString &startPoint : startPoints) {
        QDir dir(startPoint);
        for (int depth = 0; depth < 8; ++depth) {
            const QString directTemplates = dir.absoluteFilePath("templates");
            if (QFileInfo::exists(directTemplates) && QFileInfo(directTemplates).isDir())
                return QFileInfo(directTemplates).absoluteFilePath();

            const QString sourceTemplates = dir.absoluteFilePath("resources/templates");
            if (QFileInfo::exists(sourceTemplates) && QFileInfo(sourceTemplates).isDir())
                return QFileInfo(sourceTemplates).absoluteFilePath();

            if (!dir.cdUp())
                break;
        }
    }

    return {};
}

bool writeTemplateFile(const QString &srcPath,
                       const QString &dstPath,
                       const QString &projectName,
                       QString *error)
{
    QFile srcFile(srcPath);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QObject::tr("Failed to open template file: %1")
                         .arg(QDir::toNativeSeparators(srcPath));
        }
        return false;
    }

    QByteArray content = srcFile.readAll();
    if (isTemplateTextFile(srcPath)) {
        content = applyProjectNameToken(QString::fromUtf8(content), projectName).toUtf8();
    }

    QDir().mkpath(QFileInfo(dstPath).absolutePath());

    QFile dstFile(dstPath);
    if (!dstFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QObject::tr("Failed to write project file: %1")
                         .arg(QDir::toNativeSeparators(dstPath));
        }
        return false;
    }

    if (dstFile.write(content) != content.size()) {
        if (error) {
            *error = QObject::tr("Failed to fully write project file: %1")
                         .arg(QDir::toNativeSeparators(dstPath));
        }
        return false;
    }

    return true;
}

} // namespace

QVector<ProjectTemplateInfo> ProjectTemplates::availableTemplates()
{
    QVector<ProjectTemplateInfo> templates;
    const QString root = templatesDir();
    if (root.isEmpty())
        return templates;

    const QFileInfoList entries = QDir(root).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo &entry : entries) {
        ProjectTemplateInfo info;
        if (loadTemplateInfo(entry.absoluteFilePath(), &info))
            templates.append(info);
    }

    std::sort(templates.begin(), templates.end(),
              [](const ProjectTemplateInfo &lhs, const ProjectTemplateInfo &rhs) {
        if (lhs.sortOrder != rhs.sortOrder)
            return lhs.sortOrder < rhs.sortOrder;
        return lhs.name.localeAwareCompare(rhs.name) < 0;
    });

    return templates;
}

bool ProjectTemplates::templateInfo(const QString &id, ProjectTemplateInfo *info)
{
    const auto templates = availableTemplates();
    for (const auto &candidate : templates) {
        if (candidate.id == id) {
            if (info)
                *info = candidate;
            return true;
        }
    }
    return false;
}

QStringList ProjectTemplates::summaryFiles(const QString &id, const QString &projectName)
{
    ProjectTemplateInfo info;
    if (!templateInfo(id, &info))
        return {};

    QStringList rendered;
    for (const QString &path : info.summaryFiles)
        rendered.append(applyProjectNameToken(path, projectName));
    return rendered;
}

QString ProjectTemplates::templatesDir()
{
    return discoverTemplatesDirInternal();
}

bool ProjectTemplates::loadTemplateInfo(const QString &templateDir,
                                        ProjectTemplateInfo *info,
                                        QString *error)
{
    if (info)
        *info = {};

    const QString manifestPath = QDir(templateDir).filePath("template.json");
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QObject::tr("Failed to open template manifest: %1")
                         .arg(QDir::toNativeSeparators(manifestPath));
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QObject::tr("Failed to parse template manifest '%1': %2")
                         .arg(QDir::toNativeSeparators(manifestPath), parseError.errorString());
        }
        return false;
    }

    const QJsonObject obj = doc.object();
    ProjectTemplateInfo loaded;
    loaded.id = obj["id"].toString();
    loaded.name = obj["name"].toString();
    loaded.description = obj["description"].toString();
    loaded.projectType = obj["project_type"].toString();
    loaded.sortOrder = obj["sort_order"].toInt();
    loaded.path = QFileInfo(templateDir).absoluteFilePath();

    for (const auto &value : obj["summary_files"].toArray()) {
        const QString path = value.toString().trimmed();
        if (!path.isEmpty())
            loaded.summaryFiles.append(path);
    }

    if (loaded.summaryFiles.isEmpty())
        loaded.summaryFiles = collectTemplateSummaryFiles(templateDir);

    if (!loaded.isValid()) {
        if (error) {
            *error = QObject::tr("Template manifest is incomplete: %1")
                         .arg(QDir::toNativeSeparators(manifestPath));
        }
        return false;
    }

    if (info)
        *info = loaded;
    return true;
}

bool ProjectTemplates::ensureProjectSkeleton(const QString &projectDir)
{
    QDir d(projectDir);
    return d.mkpath(".")
        && d.mkpath("graphs")
        && d.mkpath("ui")
        && d.mkpath("src")
        && d.mkpath("src/ui")
        && d.mkpath("build")
        && d.mkpath("dqmods");
}

bool ProjectTemplates::copyTemplateDirectory(const QString &src,
                                             const QString &dst,
                                             const QString &projectName,
                                             QString *error)
{
    QDir srcDir(src);
    if (!srcDir.exists()) {
        if (error) {
            *error = QObject::tr("Template directory does not exist: %1")
                         .arg(QDir::toNativeSeparators(src));
        }
        return false;
    }

    QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString srcPath = it.next();
        const QString relPath = srcDir.relativeFilePath(srcPath);
        if (relPath == "template.json")
            continue;

        const QString dstPath = QDir(dst).filePath(applyProjectNameToken(relPath, projectName));
        if (!writeTemplateFile(srcPath, dstPath, projectName, error))
            return false;
    }

    return true;
}

bool ProjectTemplates::generate(const QString &id,
                                const QString &projectDir,
                                const QString &name,
                                QString *error)
{
    ProjectTemplateInfo info;
    if (!templateInfo(id, &info)) {
        if (error) {
            *error = QObject::tr("Template '%1' was not found in %2")
                         .arg(id, QDir::toNativeSeparators(templatesDir()));
        }
        return false;
    }

    if (!ensureProjectSkeleton(projectDir)) {
        if (error) {
            *error = QObject::tr("Failed to create project directories in %1")
                         .arg(QDir::toNativeSeparators(projectDir));
        }
        return false;
    }

    return copyTemplateDirectory(info.path, projectDir, name, error);
}

} // namespace DeltaQ

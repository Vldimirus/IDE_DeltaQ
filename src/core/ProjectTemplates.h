// Файловые шаблоны проектов: discovery, метаданные и копирование в проект
#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace DeltaQ {

struct ProjectTemplateInfo {
    QString id;
    QString name;
    QString description;
    QString projectType;
    QString path;
    QStringList summaryFiles;
    QString catalogRole = QStringLiteral("recommended_starter");
    QString hiddenReason;
    int sortOrder = 0;

    bool isValid() const
    {
        return !id.isEmpty()
            && !name.isEmpty()
            && !projectType.isEmpty()
            && !path.isEmpty();
    }

    bool isUserVisible() const
    {
        return catalogRole != QStringLiteral("internal_only");
    }
};

class ProjectTemplates {
public:
    static QVector<ProjectTemplateInfo> availableTemplates(bool includeHidden = false);
    static bool templateInfo(const QString &id, ProjectTemplateInfo *info);
    static QStringList summaryFiles(const QString &id, const QString &projectName);

    // Копирование шаблонных файлов по ID шаблона
    static bool generate(const QString &id,
                         const QString &projectDir,
                         const QString &name,
                         QString *error = nullptr);

    // Корень каталога templates, найденный рядом с бинарником или в source tree.
    static QString templatesDir();

private:
    static bool loadTemplateInfo(const QString &templateDir,
                                 ProjectTemplateInfo *info,
                                 QString *error = nullptr);

    static bool ensureProjectSkeleton(const QString &projectDir);
    static bool copyTemplateDirectory(const QString &src,
                                      const QString &dst,
                                      const QString &projectName,
                                      QString *error = nullptr);
};

} // namespace DeltaQ

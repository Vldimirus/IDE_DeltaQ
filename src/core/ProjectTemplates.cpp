// Копирование файловых шаблонов — реализация
#include "ProjectTemplates.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>

namespace DeltaQ {

QString ProjectTemplates::templatesDir()
{
    return QCoreApplication::applicationDirPath() + "/templates";
}

bool ProjectTemplates::copyDirectory(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    if (!srcDir.exists())
        return false;

    QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();

        // Относительный путь внутри шаблона
        QString relPath = srcDir.relativeFilePath(it.filePath());
        QString dstPath = dst + "/" + relPath;

        // Создаём промежуточные каталоги
        QDir().mkpath(QFileInfo(dstPath).absolutePath());

        // Копируем файл (перезаписываем если уже есть)
        if (QFile::exists(dstPath))
            QFile::remove(dstPath);

        if (!QFile::copy(it.filePath(), dstPath))
            return false;
    }

    return true;
}

bool ProjectTemplates::generate(const QString &type,
                                const QString &projectDir,
                                const QString &name)
{
    Q_UNUSED(name)

    QString templatePath = templatesDir() + "/" + type;

    if (!QDir(templatePath).exists())
        return false;

    return copyDirectory(templatePath, projectDir);
}

} // namespace DeltaQ

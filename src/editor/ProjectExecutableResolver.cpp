#include "ProjectExecutableResolver.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStringList>

namespace DeltaQ {

namespace {

QStringList candidateFileNames(const QString &targetName)
{
    QStringList result;
    const auto appendUnique = [&result](const QString &value) {
        if (!value.isEmpty() && !result.contains(value))
            result.append(value);
    };

    appendUnique(targetName);
    appendUnique(targetName + ".exe");

    const QString lower = targetName.toLower();
    appendUnique(lower);
    appendUnique(lower + ".exe");

    return result;
}

QStringList directCandidatePaths(const QString &buildDir, const QString &targetName)
{
    QStringList result;
    const QStringList names = candidateFileNames(targetName);
    const QStringList prefixes = {
        buildDir,
        buildDir + "/src",
        buildDir + "/Debug",
        buildDir + "/Release",
        buildDir + "/src/Debug",
        buildDir + "/src/Release"
    };

    for (const auto &prefix : prefixes) {
        for (const auto &name : names)
            result.append(QDir(prefix).filePath(name));
    }

    return result;
}

} // namespace

QString ProjectExecutableResolver::resolve(const QString &buildDir, const QString &targetName) const
{
    if (buildDir.isEmpty() || targetName.isEmpty())
        return {};

    for (const auto &path : directCandidatePaths(buildDir, targetName)) {
        if (isRunnableExecutable(path))
            return QFileInfo(path).absoluteFilePath();
    }

    const QStringList names = candidateFileNames(targetName);
    QDirIterator it(buildDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (!names.contains(info.fileName()))
            continue;
        if (!isRunnableExecutable(info.absoluteFilePath()))
            continue;
        return info.absoluteFilePath();
    }

    return {};
}

bool ProjectExecutableResolver::isRunnableExecutable(const QString &path) const
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile())
        return false;
    return info.isExecutable() || info.fileName().endsWith(".exe", Qt::CaseInsensitive);
}

} // namespace DeltaQ

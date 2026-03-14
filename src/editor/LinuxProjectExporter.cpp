#include "LinuxProjectExporter.h"
#include "ProjectExecutableResolver.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace DeltaQ {

namespace {

bool recreateDir(const QString &path, bool cleanOutput, QString *errorMessage)
{
    QDir dir(path);
    if (dir.exists() && cleanOutput && !dir.removeRecursively()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to clean export directory: %1").arg(path);
        return false;
    }

    if (!QDir().mkpath(path)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to create export directory: %1").arg(path);
        return false;
    }

    return true;
}

bool copyExecutable(const QString &sourcePath, const QString &destinationPath, QString *errorMessage)
{
    QFile::remove(destinationPath);
    if (!QFile::copy(sourcePath, destinationPath)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Failed to copy executable from %1 to %2")
                .arg(sourcePath, destinationPath);
        }
        return false;
    }

    QFile sourceFile(sourcePath);
    QFile destinationFile(destinationPath);
    if (!destinationFile.setPermissions(sourceFile.permissions())) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to preserve executable permissions for %1")
                .arg(destinationPath);
        return false;
    }

    return true;
}

bool writeLauncher(const QString &launcherPath, const QString &binaryFileName, QString *errorMessage)
{
    QFile launcher(launcherPath);
    if (!launcher.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to create launcher: %1").arg(launcherPath);
        return false;
    }

    QTextStream out(&launcher);
    out << "#!/bin/sh\n";
    out << "set -eu\n";
    out << "SCRIPT_PATH=\"$0\"\n";
    out << "case \"$SCRIPT_PATH\" in\n";
    out << "  */*) ;;\n";
    out << "  *) SCRIPT_PATH=\"./$SCRIPT_PATH\" ;;\n";
    out << "esac\n";
    out << "SCRIPT_DIR=\"${SCRIPT_PATH%/*}\"\n";
    out << "[ \"$SCRIPT_DIR\" = \"$SCRIPT_PATH\" ] && SCRIPT_DIR='.'\n";
    out << "SCRIPT_DIR=\"$(CDPATH= cd -- \"$SCRIPT_DIR\" && pwd)\"\n";
    out << "export DELTAQ_BUNDLE_ROOT=\"$SCRIPT_DIR\"\n";
    out << "export DELTAQ_ASSET_ROOT=\"$SCRIPT_DIR/assets\"\n";
    out << "export DELTAQ_FONT_PATH=\"$SCRIPT_DIR/assets/fonts/default.ttf\"\n";
    out << "export LD_LIBRARY_PATH=\"$SCRIPT_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"\n";
    out << "cd \"$SCRIPT_DIR\"\n";
    out << "exec \"$SCRIPT_DIR/bin/" << binaryFileName << "\" \"$@\"\n";
    launcher.close();

    QFileDevice::Permissions perms = launcher.permissions();
    perms |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
    perms |= QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    perms |= QFileDevice::WriteOwner;
    if (!QFile::setPermissions(launcherPath, perms)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to mark launcher as executable: %1").arg(launcherPath);
        return false;
    }

    return true;
}

bool writeManifest(const QString &manifestPath, const LinuxExportOptions &options,
                   const QString &sourceExecutablePath, const QString &exportedExecutablePath,
                   const QString &launcherPath, QString *errorMessage)
{
    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to create export manifest: %1").arg(manifestPath);
        return false;
    }

    QTextStream out(&manifest);
    out << "DeltaQ Linux Export v1\n";
    out << "Project: " << options.projectName << "\n";
    out << "Project type: " << options.projectType << "\n";
    out << "Project dir: " << options.projectDir << "\n";
    out << "Build dir: " << (options.buildDir.isEmpty() ? QDir(options.projectDir).filePath("build")
                                                        : options.buildDir) << "\n";
    out << "Source executable: " << sourceExecutablePath << "\n";
    out << "Exported executable: " << exportedExecutablePath << "\n";
    out << "Launcher: " << launcherPath << "\n";
    out << "Mode: built-project bundle\n";
    out << "Generated at (UTC): " << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << "\n";
    return true;
}

QString findBundledFontSource()
{
    const QByteArray overridePath = qgetenv("DELTAQ_EXPORT_FONT_PATH");
    const QStringList candidates = {
        overridePath.isEmpty() ? QString() : QString::fromUtf8(overridePath),
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    };

    for (const auto &candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate))
            return candidate;
    }

    return {};
}

bool parseLddLibraryLine(const QString &line, QString *libraryPath)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith("linux-vdso"))
        return false;

    const QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const auto &part : parts) {
        if (part.startsWith('/')) {
            if (libraryPath)
                *libraryPath = part;
            return true;
        }
    }

    return false;
}

QSet<QString> systemLibraryAllowList()
{
    return {
        "ld-linux-x86-64.so.2",
        "ld-linux.so.2",
        "libc.so.6",
        "libm.so.6",
        "libpthread.so.0",
        "libdl.so.2",
        "librt.so.1",
        "libgcc_s.so.1",
        "libstdc++.so.6",
        "libresolv.so.2",
        "libutil.so.1"
    };
}

bool copyFilePreservingPermissions(const QString &sourcePath, const QString &destinationPath,
                                   QString *errorMessage)
{
    QFile::remove(destinationPath);
    if (!QFile::copy(sourcePath, destinationPath)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Failed to copy file from %1 to %2")
                .arg(sourcePath, destinationPath);
        }
        return false;
    }

    QFile sourceFile(sourcePath);
    QFile destinationFile(destinationPath);
    if (!destinationFile.setPermissions(sourceFile.permissions())) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Failed to preserve permissions for %1")
                .arg(destinationPath);
        }
        return false;
    }

    return true;
}

bool copyDesktopRuntimeDependencies(const QString &executablePath, const QString &libDir,
                                    QString *errorMessage)
{
    QProcess ldd;
    ldd.start("ldd", {executablePath});
    if (!ldd.waitForStarted(30000)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to start ldd for %1").arg(executablePath);
        return false;
    }
    if (!ldd.waitForFinished(30000) || ldd.exitStatus() != QProcess::NormalExit || ldd.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QObject::tr("ldd failed for %1: %2")
                .arg(executablePath, QString::fromUtf8(ldd.readAllStandardError()));
        }
        return false;
    }

    const QSet<QString> allowList = systemLibraryAllowList();
    QSet<QString> copiedPaths;
    const QString output = QString::fromUtf8(ldd.readAllStandardOutput());
    for (const auto &line : output.split('\n')) {
        QString libraryPath;
        if (!parseLddLibraryLine(line, &libraryPath))
            continue;

        const QFileInfo info(libraryPath);
        if (!info.exists() || !info.isFile())
            continue;
        if (allowList.contains(info.fileName()))
            continue;
        if (copiedPaths.contains(info.canonicalFilePath()))
            continue;

        QString copyError;
        if (!copyFilePreservingPermissions(info.absoluteFilePath(),
                                           QDir(libDir).filePath(info.fileName()),
                                           &copyError)) {
            if (errorMessage)
                *errorMessage = copyError;
            return false;
        }
        copiedPaths.insert(info.canonicalFilePath());
    }

    return true;
}

bool copyDesktopFontBaseline(const QString &fontsDir, QString *errorMessage)
{
    const QString sourceFont = findBundledFontSource();
    if (sourceFont.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr(
                "Desktop export requires a font baseline. Set DELTAQ_EXPORT_FONT_PATH or install DejaVu/Liberation Sans.");
        }
        return false;
    }

    return copyFilePreservingPermissions(sourceFont, QDir(fontsDir).filePath("default.ttf"), errorMessage);
}

bool createBundleArchive(const QString &bundleDir, const QString &distRootDir,
                         const QString &archivePath, QString *errorMessage)
{
    QFile::remove(archivePath);

    const QString bundleName = QFileInfo(bundleDir).fileName();
    QProcess tar;
    tar.setWorkingDirectory(distRootDir);
    tar.start("tar", {"-czf", archivePath, bundleName});
    if (!tar.waitForStarted(30000)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Failed to start tar for bundle archive");
        return false;
    }

    if (!tar.waitForFinished(120000) || tar.exitStatus() != QProcess::NormalExit || tar.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QObject::tr("tar failed for %1: %2")
                .arg(bundleDir, QString::fromUtf8(tar.readAllStandardError()));
        }
        return false;
    }

    if (!QFileInfo::exists(archivePath)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Bundle archive was not created: %1").arg(archivePath);
        return false;
    }

    return true;
}

} // namespace

LinuxProjectExporter::LinuxProjectExporter(QObject *parent)
    : QObject(parent)
{
}

LinuxExportResult LinuxProjectExporter::exportBuiltProject(const LinuxExportOptions &options) const
{
    LinuxExportResult result;

    if (options.projectName.isEmpty()) {
        result.errorMessage = tr("Project name is required for Linux export");
        return result;
    }

    if (options.projectDir.isEmpty()) {
        result.errorMessage = tr("Project directory is required for Linux export");
        return result;
    }

    if (options.projectType != "console" && options.projectType != "desktop") {
        result.errorMessage = tr("Linux export v1 currently supports console and desktop projects");
        return result;
    }

    const QString buildDir = options.buildDir.isEmpty()
        ? QDir(options.projectDir).filePath("build")
        : options.buildDir;
    const QString distRootDir = options.distRootDir.isEmpty()
        ? QDir(options.projectDir).filePath("dist")
        : options.distRootDir;
    const QString bundleDir = QDir(distRootDir).filePath(options.projectName);
    const QString binDir = QDir(bundleDir).filePath("bin");
    const QString libDir = QDir(bundleDir).filePath("lib");
    const QString assetsFontsDir = QDir(bundleDir).filePath("assets/fonts");

    ProjectExecutableResolver resolver;
    const QString sourceExecutable = options.executablePath.isEmpty()
        ? resolver.resolve(buildDir, options.projectName)
        : options.executablePath;
    if (sourceExecutable.isEmpty()) {
        result.errorMessage = tr("Built executable for target '%1' was not found in %2")
            .arg(options.projectName, buildDir);
        return result;
    }

    QString ioError;
    if (!recreateDir(bundleDir, options.cleanOutput, &ioError) ||
        !QDir().mkpath(binDir) ||
        !QDir().mkpath(libDir) ||
        !QDir().mkpath(assetsFontsDir)) {
        result.errorMessage = ioError.isEmpty()
            ? tr("Failed to prepare export bundle directories")
            : ioError;
        return result;
    }

    const QString exportedExecutable = QDir(binDir).filePath(options.projectName + ".bin");
    if (!copyExecutable(sourceExecutable, exportedExecutable, &ioError)) {
        result.errorMessage = ioError;
        return result;
    }

    if (options.projectType == "desktop") {
        if (!copyDesktopRuntimeDependencies(sourceExecutable, libDir, &ioError)) {
            result.errorMessage = ioError;
            return result;
        }
        if (!copyDesktopFontBaseline(assetsFontsDir, &ioError)) {
            result.errorMessage = ioError;
            return result;
        }
    }

    const QString launcherPath = QDir(bundleDir).filePath(options.projectName);
    if (!writeLauncher(launcherPath, QFileInfo(exportedExecutable).fileName(), &ioError)) {
        result.errorMessage = ioError;
        return result;
    }

    const QString manifestPath = QDir(bundleDir).filePath("EXPORT_INFO.txt");
    if (!writeManifest(manifestPath, options, sourceExecutable, exportedExecutable, launcherPath, &ioError)) {
        result.errorMessage = ioError;
        return result;
    }

    result.success = true;
    result.bundleDir = bundleDir;
    result.launcherPath = launcherPath;
    result.sourceExecutablePath = sourceExecutable;
    result.exportedExecutablePath = exportedExecutable;
    result.manifestPath = manifestPath;

    if (options.packageArchive) {
        const QString archiveBaseName = options.archiveBaseName.isEmpty()
            ? options.projectName + ".tar.gz"
            : options.archiveBaseName;
        const QString archivePath = QDir(distRootDir).filePath(archiveBaseName);
        if (!createBundleArchive(bundleDir, distRootDir, archivePath, &ioError)) {
            result.success = false;
            result.errorMessage = ioError;
            return result;
        }
        result.archivePath = archivePath;
    }

    return result;
}

} // namespace DeltaQ

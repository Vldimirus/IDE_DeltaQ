#include "../../src/editor/LinuxProjectExporter.h"
#include "../../src/editor/CMakeGenerator.h"
#include "../../src/codegen/PreBuildProcessor.h"
#include "../../src/core/GraphStore.h"
#include "../../src/core/ModuleRegistry.h"
#include "../../src/core/UILayoutStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

using namespace DeltaQ;

class TestLinuxProjectExporter : public QObject {
    Q_OBJECT

private:
    static QString repoRootPath()
    {
        QStringList candidates = {
            QDir::cleanPath(QString::fromUtf8(DQ_TEST_SOURCE_DIR)),
            QDir::cleanPath(QDir::currentPath()),
            QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath("..")),
            QDir::cleanPath(QDir(QDir::currentPath()).absoluteFilePath("../.."))
        };

        if (QCoreApplication::instance()) {
            const QDir appDir(QCoreApplication::applicationDirPath());
            candidates.append(QDir::cleanPath(appDir.absoluteFilePath("../..")));
            candidates.append(QDir::cleanPath(appDir.absoluteFilePath("../../..")));
            candidates.append(QDir::cleanPath(appDir.absoluteFilePath("../../../..")));
        }

        for (const auto &candidate : candidates) {
            if (QFile::exists(candidate + "/modules/core/pack.json") &&
                QFile::exists(candidate + "/resources/examples/desktop_ui_flow/desktop_ui_flow.dqproj")) {
                return candidate;
            }
        }

        return {};
    }

    static bool copyDirectory(const QString &src, const QString &dst)
    {
        QDir srcDir(src);
        if (!srcDir.exists())
            return false;

        QDir().mkpath(dst);
        QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();

            const QString relPath = srcDir.relativeFilePath(it.filePath());
            const QString dstPath = dst + "/" + relPath;
            QDir().mkpath(QFileInfo(dstPath).absolutePath());
            if (QFile::exists(dstPath))
                QFile::remove(dstPath);
            if (!QFile::copy(it.filePath(), dstPath))
                return false;
        }

        return true;
    }

    static void writeConsoleProject(const QString &projectDir)
    {
        QDir().mkpath(projectDir);
        QFile mainFile(projectDir + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&mainFile);
        out << "#include <stdio.h>\n";
        out << "int main(void) {\n";
        out << "    puts(\"Hello from exported bundle\");\n";
        out << "    return 0;\n";
        out << "}\n";
    }

    static bool runProcess(const QString &program, const QStringList &arguments,
                           const QString &workingDir, QByteArray *stdoutData,
                           QByteArray *stderrData, int timeoutMs = 120000,
                           const QProcessEnvironment *environment = nullptr)
    {
        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.setWorkingDirectory(workingDir);
        if (environment)
            process.setProcessEnvironment(*environment);
        process.start();
        if (!process.waitForStarted(30000))
            return false;
        if (!process.waitForFinished(timeoutMs))
            return false;

        if (stdoutData)
            *stdoutData = process.readAllStandardOutput();
        if (stderrData)
            *stderrData = process.readAllStandardError();
        return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    }

    static void buildConsoleProject(const QString &projectDir, const QString &targetName)
    {
        CMakeGenerator generator;
        generator.generate(projectDir, targetName, "17", "20", {}, "console");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for export test project");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));
    }

    static QString extractArchive(const QString &archivePath, const QString &extractRoot)
    {
        QByteArray stdoutData;
        QByteArray stderrData;
        if (!runProcess("tar", {"-xzf", archivePath, "-C", extractRoot},
                        extractRoot, &stdoutData, &stderrData)) {
            QTest::qFail(qPrintable(QString::fromUtf8(stdoutData + stderrData)), __FILE__, __LINE__);
            return {};
        }

        QDir root(extractRoot);
        const QStringList entries = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (entries.size() != 1) {
            QTest::qFail(qPrintable(QString("Expected one extracted directory, got %1").arg(entries.size())),
                         __FILE__, __LINE__);
            return {};
        }
        return root.filePath(entries.first());
    }

    static void verifyExportBundleScripts(const QString &repoRoot, const QString &bundleDir,
                                          const QString &archivePath, const QString &workingDir)
    {
        QByteArray stdoutData;
        QByteArray stderrData;
        QVERIFY2(runProcess("bash",
                            {repoRoot + "/scripts/verify_project_export_bundle.sh", bundleDir},
                            workingDir, &stdoutData, &stderrData),
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));
        stdoutData.clear();
        stderrData.clear();
        QVERIFY2(runProcess("bash",
                            {repoRoot + "/scripts/verify_project_export_archive.sh", archivePath},
                            workingDir, &stdoutData, &stderrData),
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));
    }

    static void verifyCleanEnvironmentScript(const QString &repoRoot,
                                             const QString &bundleDir,
                                             const QString &workingDir,
                                             const QString &expectedText = {},
                                             const QProcessEnvironment *environment = nullptr)
    {
        QByteArray stdoutData;
        QByteArray stderrData;
        QStringList arguments = {
            repoRoot + "/scripts/verify_project_export_clean_env.sh",
            bundleDir
        };
        if (!expectedText.isEmpty())
            arguments << "--expect-text" << expectedText;

        QVERIFY2(runProcess("bash",
                            arguments,
                            workingDir,
                            &stdoutData,
                            &stderrData,
                            120000,
                            environment),
                 qPrintable(QString::fromUtf8(stdoutData + stderrData)));
    }

private slots:
    void exportsBuiltConsoleProjectIntoHandoffBundle()
    {
        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        QTemporaryDir projectRoot;
        QTemporaryDir handoffRoot;
        QTemporaryDir extractRoot;
        QVERIFY(projectRoot.isValid());
        QVERIFY(handoffRoot.isValid());
        QVERIFY(extractRoot.isValid());

        const QString projectDir = projectRoot.path() + "/console_export_project";
        const QString targetName = "ConsoleExportApp";
        writeConsoleProject(projectDir);
        buildConsoleProject(projectDir, targetName);

        LinuxProjectExporter exporter;
        LinuxExportOptions options;
        options.projectDir = projectDir;
        options.projectName = targetName;
        options.projectType = "console";
        options.distRootDir = handoffRoot.path();
        options.packageArchive = true;

        const LinuxExportResult result = exporter.exportBuiltProject(options);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QFileInfo::exists(result.launcherPath));
        QVERIFY(QFileInfo::exists(result.exportedExecutablePath));
        QVERIFY(QFileInfo::exists(result.archivePath));
        QVERIFY(QFileInfo(result.launcherPath).isExecutable());
        QVERIFY(QFileInfo(result.exportedExecutablePath).isExecutable());

        verifyExportBundleScripts(repoRoot, result.bundleDir, result.archivePath, handoffRoot.path());

        QFile manifest(result.manifestPath);
        QVERIFY(manifest.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString manifestText = QString::fromUtf8(manifest.readAll());
        QVERIFY(manifestText.contains("DeltaQ Linux Export v1"));
        QVERIFY(manifestText.contains(targetName));
        QVERIFY(manifestText.contains("Mode: built-project bundle"));

        QVERIFY(QDir(projectDir).removeRecursively());
        QVERIFY(QDir(result.bundleDir).removeRecursively());

        const QString extractedBundleDir = extractArchive(result.archivePath, extractRoot.path());
        const QString extractedLauncherPath = QDir(extractedBundleDir).filePath(targetName);

        verifyCleanEnvironmentScript(repoRoot,
                                     extractedBundleDir,
                                     extractRoot.path(),
                                     "Hello from exported bundle");

        QProcess app;
        app.setProcessEnvironment(QProcessEnvironment());
        app.setProgram(extractedLauncherPath);
        app.setWorkingDirectory(extractRoot.path());
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Exported launcher failed to start");
        QVERIFY2(app.waitForFinished(30000), "Exported launcher did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2").arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));
        QVERIFY2(stdoutText.contains("Hello from exported bundle"), qPrintable(runtimeLog));
    }

    void exportsDesktopExampleIntoRunnableBundle()
    {
        const QString repoRoot = repoRootPath();
        QVERIFY2(!repoRoot.isEmpty(), "Repository root was not found from test binary location");

        QTemporaryDir workspaceRoot;
        QTemporaryDir handoffRoot;
        QVERIFY(workspaceRoot.isValid());
        QVERIFY(handoffRoot.isValid());

        const QString projectDir = workspaceRoot.path() + "/desktop_ui_flow";
        QVERIFY(copyDirectory(repoRoot + "/resources/examples/desktop_ui_flow", projectDir));

        ModuleRegistry registry;
        registry.loadGlobalModules(repoRoot + "/modules");

        GraphStore graphStore;
        UILayoutStore layoutStore;
        QVERIFY(graphStore.loadFromDirectory(projectDir));
        QVERIFY(layoutStore.loadFromDirectory(projectDir));

        PreBuildProcessor processor(&registry, &graphStore, &layoutStore);
        const PreBuildResult prebuild = processor.process(projectDir);
        QVERIFY2(prebuild.success, qPrintable(prebuild.errors.join('\n')));

        CMakeGenerator generator;
        const QString targetName = "DesktopUIFlow";
        generator.generate(projectDir, targetName, "17", "20", {}, "desktop");
        QVERIFY2(generator.configure(projectDir), "CMake configure failed for desktop export example");

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build", "--parallel"}, projectDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        LinuxProjectExporter exporter;
        LinuxExportOptions options;
        options.projectDir = projectDir;
        options.projectName = targetName;
        options.projectType = "desktop";
        options.distRootDir = handoffRoot.path();
        options.packageArchive = true;

        const LinuxExportResult result = exporter.exportBuiltProject(options);
        QVERIFY2(result.success, qPrintable(result.errorMessage));
        QVERIFY(QFileInfo::exists(result.launcherPath));
        QVERIFY(QFileInfo::exists(result.exportedExecutablePath));
        QVERIFY(QFileInfo::exists(result.archivePath));
        QVERIFY(QFileInfo::exists(QDir(result.bundleDir).filePath("assets/fonts/default.ttf")));
        QVERIFY(!QDir(result.bundleDir + "/lib").entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());

        verifyExportBundleScripts(repoRoot, result.bundleDir, result.archivePath, handoffRoot.path());

        QVERIFY(QDir(projectDir).removeRecursively());
        QVERIFY(QDir(result.bundleDir).removeRecursively());

        QTemporaryDir extractRoot;
        QVERIFY(extractRoot.isValid());
        const QString extractedBundleDir = extractArchive(result.archivePath, extractRoot.path());
        const QString extractedLauncherPath = QDir(extractedBundleDir).filePath(targetName);

        QProcessEnvironment cleanEnv;
        cleanEnv.insert("SDL_VIDEODRIVER", "dummy");
        cleanEnv.insert("SDL_RENDER_DRIVER", "software");
        cleanEnv.insert("DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS", "1200");
        verifyCleanEnvironmentScript(repoRoot,
                                     extractedBundleDir,
                                     extractRoot.path(),
                                     {},
                                     &cleanEnv);

        QProcess app;
        app.setProcessEnvironment(cleanEnv);
        app.setProgram(extractedLauncherPath);
        app.setWorkingDirectory(extractRoot.path());
        app.start();
        QVERIFY2(app.waitForStarted(30000), "Exported desktop launcher failed to start");
        QVERIFY2(app.waitForFinished(15000), "Exported desktop launcher did not finish in time");

        const QString stdoutText = QString::fromUtf8(app.readAllStandardOutput());
        const QString stderrText = QString::fromUtf8(app.readAllStandardError());
        const QString runtimeLog = QString("STDOUT:\n%1\nSTDERR:\n%2").arg(stdoutText, stderrText);
        QVERIFY2(app.exitStatus() == QProcess::NormalExit && app.exitCode() == 0,
                 qPrintable(runtimeLog));
    }
};

QTEST_APPLESS_MAIN(TestLinuxProjectExporter)
#include "test_LinuxProjectExporter.moc"

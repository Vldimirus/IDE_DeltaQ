// Тесты controlled fixture library для imported pack showcase
#include <QTest>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>

class TestImportedPackFixture : public QObject {
    Q_OBJECT

private:
    // Копирует fixture library во временную директорию, чтобы тест не трогал repo files.
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

    // Запускает внешний процесс и возвращает stdout/stderr для диагностики падения теста.
    static bool runProcess(const QString &program, const QStringList &arguments,
                           const QString &workingDir, QByteArray *stdoutData,
                           QByteArray *stderrData, int timeoutMs = 120000)
    {
        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.setWorkingDirectory(workingDir);
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

private slots:
    void miniSensorSdkBuildsAndRunsDeterministically()
    {
        const QString fixtureCMake =
            QFINDTESTDATA("../../resources/examples/imported_pack_sensor_sdk/vendor/mini_sensor_sdk/CMakeLists.txt");
        QVERIFY2(!fixtureCMake.isEmpty(), "Fixture CMakeLists.txt was not found");

        const QString fixtureDir = QFileInfo(fixtureCMake).absolutePath();

        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString workDir = tmpDir.path() + "/mini_sensor_sdk";
        QVERIFY(copyDirectory(fixtureDir, workDir));

        QByteArray configureOut;
        QByteArray configureErr;
        QVERIFY2(runProcess("cmake", {"-S", ".", "-B", "build"}, workDir,
                            &configureOut, &configureErr),
                 qPrintable(QString::fromUtf8(configureOut + configureErr)));

        QByteArray buildOut;
        QByteArray buildErr;
        QVERIFY2(runProcess("cmake", {"--build", "build"}, workDir,
                            &buildOut, &buildErr),
                 qPrintable(QString::fromUtf8(buildOut + buildErr)));

        const QString libraryPath = workDir + "/build/libmini_sensor_sdk.a";
        QVERIFY2(QFile::exists(libraryPath),
                 qPrintable(QString("Built library was not found: %1").arg(libraryPath)));

        // Маленькая harness-программа проверяет API как внешний потребитель библиотеки.
        const QString harnessPath = workDir + "/harness.c";
        QFile harnessFile(harnessPath);
        QVERIFY(harnessFile.open(QIODevice::WriteOnly | QIODevice::Text));
        harnessFile.write(
            "#include <stdio.h>\n"
            "#include \"mini_sensor_sdk.h\"\n"
            "\n"
            "int main(void) {\n"
            "    mini_sensor_context *ctx = mini_sensor_init(\"sensor-A\");\n"
            "    if (!ctx) {\n"
            "        return 1;\n"
            "    }\n"
            "    printf(\"READ=%d\\n\", mini_sensor_read(ctx));\n"
            "    printf(\"SCALED=%d\\n\", mini_sensor_scale(ctx, 3));\n"
            "    printf(\"ERROR=%d\\n\", mini_sensor_scale(ctx, 0));\n"
            "    printf(\"LAST_ERROR=%s\\n\", mini_sensor_last_error(ctx));\n"
            "    mini_sensor_shutdown(ctx);\n"
            "    return 0;\n"
            "}\n");
        harnessFile.close();

        QByteArray harnessBuildOut;
        QByteArray harnessBuildErr;
        QVERIFY2(runProcess("gcc",
                            {harnessPath, libraryPath, "-I", workDir + "/include",
                             "-o", workDir + "/harness"},
                            workDir, &harnessBuildOut, &harnessBuildErr),
                 qPrintable(QString::fromUtf8(harnessBuildOut + harnessBuildErr)));

        QByteArray appOut;
        QByteArray appErr;
        QVERIFY2(runProcess(workDir + "/harness", {}, workDir, &appOut, &appErr),
                 qPrintable(QString::fromUtf8(appOut + appErr)));

        const QString runtimeLog = QString::fromUtf8(appOut + appErr);
        QVERIFY2(runtimeLog.contains("READ=48"), qPrintable(runtimeLog));
        QVERIFY2(runtimeLog.contains("SCALED=144"), qPrintable(runtimeLog));
        QVERIFY2(runtimeLog.contains("ERROR=-1"), qPrintable(runtimeLog));
        QVERIFY2(runtimeLog.contains("LAST_ERROR=scale factor must be positive"),
                 qPrintable(runtimeLog));
    }
};

QTEST_APPLESS_MAIN(TestImportedPackFixture)
#include "test_ImportedPackFixture.moc"

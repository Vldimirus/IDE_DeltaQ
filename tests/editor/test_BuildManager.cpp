#include "../../src/editor/BuildManager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

#include "../../src/editor/CMakeGenerator.h"

using namespace DeltaQ;

class TestBuildManager : public QObject {
    Q_OBJECT

private:
    static void writeMinimalProject(const QString &projectDir)
    {
        QFile mainFile(projectDir + "/main.c");
        QVERIFY(mainFile.open(QIODevice::WriteOnly | QIODevice::Text));
        mainFile.write("int main(void) { return 0; }\n");
        mainFile.close();
    }

    static ProjectBuildRequest requestForConfiguredTree(BuildManager &manager,
                                                        const QString &projectDir,
                                                        const QString &buildDir,
                                                        const QString &fingerprint)
    {
        ProjectBuildRequest request;
        request.projectDir = projectDir;
        request.projectName = "BuildManagerTest";
        request.projectType = "console";
        request.cStandard = "c17";
        request.cxxStandard = "c++20";
        request.toolchain.valid = true;
        request.toolchain.cmakePath = "cmake";
        request.toolchain.generatorDisplayName =
            manager.cacheValue(buildDir + "/CMakeCache.txt", "CMAKE_GENERATOR");
        request.toolchain.generator =
            request.toolchain.generatorDisplayName == "Ninja" ? "ninja" : "unix_makefiles";
        request.toolchain.buildProfile = "Debug";
        request.toolchain.cCompilerPath =
            manager.cacheValue(buildDir + "/CMakeCache.txt", "CMAKE_C_COMPILER");
        request.toolchain.cxxCompilerPath =
            manager.cacheValue(buildDir + "/CMakeCache.txt", "CMAKE_CXX_COMPILER");
        request.toolchain.fingerprint = fingerprint;
        return request;
    }

    static bool configureProject(const QString &projectDir,
                                 BuildManager &manager,
                                 ProjectBuildRequest *request,
                                 QString *buildDir)
    {
        CMakeGenerator generator;
        generator.generate(projectDir, "BuildManagerTest", "c17", "c++20", {}, "console");
        if (!generator.configure(projectDir))
            return false;
        if (buildDir)
            *buildDir = projectDir + "/build";
        if (request) {
            *request = requestForConfiguredTree(manager, projectDir, projectDir + "/build",
                                                "sha256:configured");
            QFile fingerprintFile(manager.configuredFingerprintPath(projectDir + "/build"));
            if (!fingerprintFile.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            fingerprintFile.write(request->toolchain.fingerprint.toUtf8());
        }
        return true;
    }

private slots:
    void reconfiguresWhenBuildArtifactIsMissing()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        writeMinimalProject(tmpDir.path());

        BuildManager manager;
        QString buildDir;
        ProjectBuildRequest request;
        QVERIFY(configureProject(tmpDir.path(), manager, &request, &buildDir));

        QString reason;
        QVERIFY(!manager.shouldConfigure(request, &reason));
        QCOMPARE(reason, QString());

        const QString artifact = manager.expectedBuildArtifact(buildDir);
        QVERIFY(!artifact.isEmpty());
        QVERIFY(QFile::exists(artifact));
        QVERIFY(QFile::remove(artifact));
        QVERIFY(manager.shouldConfigure(request, &reason));
        QVERIFY(reason.contains("Missing generated build file"));
    }

    void reconfiguresWhenCMakeListsChanges()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        writeMinimalProject(tmpDir.path());

        BuildManager manager;
        QString buildDir;
        ProjectBuildRequest request;
        QVERIFY(configureProject(tmpDir.path(), manager, &request, &buildDir));

        QString reason;
        QVERIFY(!manager.shouldConfigure(request, &reason));

        QFile cmakeFile(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmakeFile.exists());
        QTest::qWait(1200);
        QVERIFY(cmakeFile.open(QIODevice::Append | QIODevice::Text));
        QTextStream out(&cmakeFile);
        out << "\n# touched by test\n";
        cmakeFile.close();

        QVERIFY(manager.shouldConfigure(request, &reason));
        QVERIFY(reason.contains("newer than the last configure"));
    }

    void reconfiguresWhenFingerprintChanges()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        writeMinimalProject(tmpDir.path());

        BuildManager manager;
        QString buildDir;
        ProjectBuildRequest request;
        QVERIFY(configureProject(tmpDir.path(), manager, &request, &buildDir));

        QString reason;
        QVERIFY(!manager.shouldConfigure(request, &reason));

        request.toolchain.fingerprint = "sha256:new";
        QVERIFY(manager.shouldConfigure(request, &reason));
        QVERIFY(reason.contains("fingerprint"));
    }

    void configureArgumentsReflectResolvedToolchain()
    {
        BuildManager manager;
        ProjectBuildRequest request;
        request.projectDir = "/tmp/project";
        request.projectName = "ArgsTest";
        request.toolchain.valid = true;
        request.toolchain.cmakePath = "/opt/cmake/bin/cmake";
        request.toolchain.generator = "ninja";
        request.toolchain.generatorDisplayName = "Ninja";
        request.toolchain.buildProfile = "Release";
        request.toolchain.cCompilerPath = "/opt/toolchains/clang";
        request.toolchain.cxxCompilerPath = "/opt/toolchains/clang++";
        request.toolchain.builderPath = "/opt/toolchains/ninja";

        const QStringList args = manager.configureArguments(request);
        QCOMPARE(args.first(), QString("/tmp/project"));
        QVERIFY(args.contains("-G"));
        QVERIFY(args.contains("Ninja"));
        QVERIFY(args.contains("-DCMAKE_BUILD_TYPE=Release"));
        QVERIFY(args.contains("-DCMAKE_C_COMPILER=/opt/toolchains/clang"));
        QVERIFY(args.contains("-DCMAKE_CXX_COMPILER=/opt/toolchains/clang++"));
        QVERIFY(args.contains("-DCMAKE_MAKE_PROGRAM=/opt/toolchains/ninja"));
    }
};

QTEST_APPLESS_MAIN(TestBuildManager)
#include "test_BuildManager.moc"

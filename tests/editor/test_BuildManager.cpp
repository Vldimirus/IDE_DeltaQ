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

    static bool configureProject(const QString &projectDir, QString *buildDir)
    {
        CMakeGenerator generator;
        generator.generate(projectDir, "BuildManagerTest", "17", "20", {}, "console");
        if (!generator.configure(projectDir))
            return false;
        if (buildDir)
            *buildDir = projectDir + "/build";
        return true;
    }

private slots:
    void reconfiguresWhenBuildArtifactIsMissing()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        writeMinimalProject(tmpDir.path());

        QString buildDir;
        QVERIFY(configureProject(tmpDir.path(), &buildDir));

        BuildManager manager;
        QString reason;
        QVERIFY(!manager.shouldConfigure(tmpDir.path(), buildDir, &reason));
        QCOMPARE(reason, QString());

        const QString artifact = manager.expectedBuildArtifact(buildDir);
        QVERIFY(!artifact.isEmpty());
        QVERIFY(QFile::exists(artifact));
        QVERIFY(QFile::remove(artifact));
        QVERIFY(manager.shouldConfigure(tmpDir.path(), buildDir, &reason));
        QVERIFY(reason.contains("Missing generated build file"));
    }

    void reconfiguresWhenCMakeListsChanges()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        writeMinimalProject(tmpDir.path());

        QString buildDir;
        QVERIFY(configureProject(tmpDir.path(), &buildDir));

        BuildManager manager;
        QString reason;
        QVERIFY(!manager.shouldConfigure(tmpDir.path(), buildDir, &reason));

        QFile cmakeFile(tmpDir.path() + "/CMakeLists.txt");
        QVERIFY(cmakeFile.exists());
        QTest::qWait(1200);
        QVERIFY(cmakeFile.open(QIODevice::Append | QIODevice::Text));
        QTextStream out(&cmakeFile);
        out << "\n# touched by test\n";
        cmakeFile.close();

        QVERIFY(manager.shouldConfigure(tmpDir.path(), buildDir, &reason));
        QVERIFY(reason.contains("newer than the last configure"));
    }
};

QTEST_APPLESS_MAIN(TestBuildManager)
#include "test_BuildManager.moc"

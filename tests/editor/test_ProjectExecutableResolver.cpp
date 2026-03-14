#include "../../src/editor/ProjectExecutableResolver.h"

#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace DeltaQ;

class TestProjectExecutableResolver : public QObject {
    Q_OBJECT

private:
    static void writeExecutableFile(const QString &path)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("#!/bin/sh\nexit 0\n");
        file.close();

        QFileDevice::Permissions perms = file.permissions();
        perms |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        perms |= QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;
        perms |= QFileDevice::WriteOwner;
        QVERIFY(QFile::setPermissions(path, perms));
    }

private slots:
    void resolvesNestedExecutableByTargetName()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString buildDir = tmpDir.path() + "/build";
        writeExecutableFile(buildDir + "/src/MyConsoleApp");

        ProjectExecutableResolver resolver;
        const QString executable = resolver.resolve(buildDir, "MyConsoleApp");

        QVERIFY(!executable.isEmpty());
        QVERIFY(executable.endsWith("/src/MyConsoleApp"));
    }

    void ignoresBuildSystemMarkers()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        const QString buildDir = tmpDir.path() + "/build";
        QDir().mkpath(buildDir);

        QFile makefile(buildDir + "/Makefile");
        QVERIFY(makefile.open(QIODevice::WriteOnly | QIODevice::Text));
        makefile.write("all:\n");
        makefile.close();

        QFile ninja(buildDir + "/build.ninja");
        QVERIFY(ninja.open(QIODevice::WriteOnly | QIODevice::Text));
        ninja.write("rule all\n");
        ninja.close();

        ProjectExecutableResolver resolver;
        QVERIFY(resolver.resolve(buildDir, "Makefile").isEmpty());
        QVERIFY(resolver.resolve(buildDir, "ExampleApp").isEmpty());
    }
};

QTEST_APPLESS_MAIN(TestProjectExecutableResolver)
#include "test_ProjectExecutableResolver.moc"

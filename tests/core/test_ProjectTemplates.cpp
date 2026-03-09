#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "../../src/core/ProjectTemplates.h"

using namespace DeltaQ;

class TestProjectTemplates : public QObject {
    Q_OBJECT

private slots:
    void discoversManifestTemplatesInDisplayOrder()
    {
        const auto templates = ProjectTemplates::availableTemplates();
        QStringList ids;
        for (const auto &tmpl : templates)
            ids.append(tmpl.id);

        QVERIFY(ids.contains("console"));
        QVERIFY(ids.contains("console_counter"));
        QVERIFY(ids.contains("desktop_empty"));
        QVERIFY(ids.contains("desktop"));
        QVERIFY(ids.contains("desktop_text_editor"));
        QVERIFY(ids.contains("desktop_mdi"));
        QVERIFY(ids.indexOf("console") < ids.indexOf("console_counter"));
        QVERIFY(ids.indexOf("console_counter") < ids.indexOf("desktop_empty"));
        QVERIFY(ids.indexOf("desktop_empty") < ids.indexOf("desktop"));
        QVERIFY(ids.indexOf("desktop") < ids.indexOf("desktop_text_editor"));
        QVERIFY(ids.indexOf("desktop_text_editor") < ids.indexOf("desktop_mdi"));
        QVERIFY(!ids.contains("internal_console_graph"));
    }

    void generateReplacesProjectNameInFiles()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QString error;
        QVERIFY2(ProjectTemplates::generate("console",
                                            tmpDir.path() + "/HelloProject",
                                            "HelloProject",
                                            &error),
                 qPrintable(error));

        QFile projectFile(tmpDir.path() + "/HelloProject/HelloProject.dqproj");
        QVERIFY(projectFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString projectJson = QString::fromUtf8(projectFile.readAll());
        QVERIFY(projectJson.contains("\"name\": \"HelloProject\""));
        QVERIFY(projectJson.contains("\"type\": \"console\""));

        QFile mainFile(tmpDir.path() + "/HelloProject/src/main.c");
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString mainCode = QString::fromUtf8(mainFile.readAll());
        QVERIFY(mainCode.contains("Hello, world!"));

        QCOMPARE(ProjectTemplates::summaryFiles("console", "HelloProject"),
                 QStringList({"HelloProject.dqproj", "src/main.c"}));
    }

    void generateCopiesNestedDesktopFiles()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QString error;
        QVERIFY2(ProjectTemplates::generate("desktop",
                                            tmpDir.path() + "/UiProject",
                                            "UiProject",
                                            &error),
                 qPrintable(error));

        QVERIFY(QFile::exists(tmpDir.path() + "/UiProject/UiProject.dqproj"));
        QVERIFY(QFile::exists(tmpDir.path() + "/UiProject/graphs/main.dqgraph"));
        QVERIFY(QFile::exists(tmpDir.path() + "/UiProject/ui/window1.dqui"));
        QVERIFY(QFile::exists(tmpDir.path() + "/UiProject/src/ui/window1_events.c"));
    }
};

QTEST_MAIN(TestProjectTemplates)
#include "test_ProjectTemplates.moc"

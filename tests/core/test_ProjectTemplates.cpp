#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include "../../src/core/ProjectTemplates.h"

using namespace DeltaQ;

namespace {

QJsonObject loadJsonObject(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    return QJsonDocument::fromJson(file.readAll()).object();
}

QJsonObject findFirstNodeByModuleId(const QJsonObject &graph, const QString &moduleId)
{
    const QJsonArray nodes = graph.value("nodes").toArray();
    for (const QJsonValue &value : nodes) {
        const QJsonObject node = value.toObject();
        if (node.value("module_id").toString() == moduleId)
            return node;
    }

    return {};
}

QPointF nodePosition(const QJsonObject &graph, const QString &moduleId)
{
    const QJsonObject node = findFirstNodeByModuleId(graph, moduleId);
    const QJsonObject position = node.value("position").toObject();
    return QPointF(position.value("x").toDouble(), position.value("y").toDouble());
}

} // namespace

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

        const QJsonObject graph = loadJsonObject(tmpDir.path() + "/UiProject/graphs/main.dqgraph");
        const QPointF sdlInit = nodePosition(graph, "core.desktop.sdl_init");
        const QPointF ttfInit = nodePosition(graph, "core.desktop.ttf_init");
        const QPointF createWindow = nodePosition(graph, "core.desktop.create_window");
        const QPointF createRenderer = nodePosition(graph, "core.desktop.create_renderer");
        const QPointF uiInit = nodePosition(graph, "core.desktop.ui_init");
        const QPointF eventLoop = nodePosition(graph, "core.desktop.event_loop");
        const QPointF cleanupFont = nodePosition(graph, "core.desktop.ui_cleanup_font");

        QCOMPARE(sdlInit.y(), 140.0);
        QCOMPARE(ttfInit.y(), 140.0);
        QCOMPARE(createWindow.y(), 140.0);
        QCOMPARE(createRenderer.y(), 140.0);
        QCOMPARE(uiInit.y(), 140.0);
        QCOMPARE(eventLoop.y(), 140.0);
        QCOMPARE(cleanupFont.y(), 140.0);

        QVERIFY(sdlInit.x() < ttfInit.x());
        QVERIFY(ttfInit.x() < createWindow.x());
        QVERIFY(createWindow.x() < createRenderer.x());
        QVERIFY(createRenderer.x() < uiInit.x());
        QVERIFY(uiInit.x() < eventLoop.x());
        QVERIFY(eventLoop.x() < cleanupFont.x());
    }

    void generateCopiesDesktopTextEditorStarterFiles()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());

        QString error;
        QVERIFY2(ProjectTemplates::generate("desktop_text_editor",
                                            tmpDir.path() + "/TextPadProject",
                                            "TextPadProject",
                                            &error),
                 qPrintable(error));

        QVERIFY(QFile::exists(tmpDir.path() + "/TextPadProject/TextPadProject.dqproj"));
        QVERIFY(QFile::exists(tmpDir.path() + "/TextPadProject/graphs/main.dqgraph"));
        QVERIFY(QFile::exists(tmpDir.path() + "/TextPadProject/ui/window1.dqui"));
        QVERIFY(QFile::exists(tmpDir.path() + "/TextPadProject/src/ui/window1_events.c"));
        QVERIFY(QFile::exists(tmpDir.path() + "/TextPadProject/src/ui/window1_events.h"));

        QFile graphSourceFile(tmpDir.path() + "/TextPadProject/graphs/main.dqgraph");
        QVERIFY(graphSourceFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString graphSource = QString::fromUtf8(graphSourceFile.readAll());
        QVERIFY(graphSource.contains("\"ui_state\""));
        QVERIFY(!graphSource.contains("\"font\""));

        const QJsonObject graph = loadJsonObject(tmpDir.path() + "/TextPadProject/graphs/main.dqgraph");
        const QJsonObject titleNode = findFirstNodeByModuleId(graph, "core.io.string_constant");
        const QString titleValue = titleNode.value("properties").toObject().value("value").toString();
        QCOMPARE(titleValue, QString("\"Text Pad\""));

        const QPointF sdlInit = nodePosition(graph, "core.desktop.sdl_init");
        const QPointF ttfInit = nodePosition(graph, "core.desktop.ttf_init");
        const QPointF createWindow = nodePosition(graph, "core.desktop.create_window");
        const QPointF createRenderer = nodePosition(graph, "core.desktop.create_renderer");
        const QPointF uiInit = nodePosition(graph, "core.desktop.ui_init");
        const QPointF eventLoop = nodePosition(graph, "core.desktop.event_loop");

        QCOMPARE(sdlInit.y(), 140.0);
        QCOMPARE(ttfInit.y(), 140.0);
        QCOMPARE(createWindow.y(), 140.0);
        QCOMPARE(createRenderer.y(), 140.0);
        QCOMPARE(uiInit.y(), 140.0);
        QCOMPARE(eventLoop.y(), 140.0);

        QVERIFY(sdlInit.x() < ttfInit.x());
        QVERIFY(ttfInit.x() < createWindow.x());
        QVERIFY(createWindow.x() < createRenderer.x());
        QVERIFY(createRenderer.x() < uiInit.x());
        QVERIFY(uiInit.x() < eventLoop.x());

        QFile layoutFile(tmpDir.path() + "/TextPadProject/ui/window1.dqui");
        QVERIFY(layoutFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString layoutJson = QString::fromUtf8(layoutFile.readAll());
        QVERIFY(layoutJson.contains("\"txtDocument\""));
        QVERIFY(layoutJson.contains("\"btnAppendLine\""));
        QVERIFY(layoutJson.contains("\"lblStatus\""));
    }
};

QTEST_MAIN(TestProjectTemplates)
#include "test_ProjectTemplates.moc"

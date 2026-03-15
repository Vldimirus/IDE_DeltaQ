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

QJsonObject findFirstChildByName(const QJsonObject &parent, const QString &name)
{
    const QJsonArray children = parent.value("children").toArray();
    for (const QJsonValue &value : children) {
        const QJsonObject child = value.toObject();
        if (child.value("name").toString() == name)
            return child;
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
        QVERIFY(ids.contains("desktop"));
        QVERIFY(ids.contains("desktop_text_editor"));
        QVERIFY(ids.indexOf("console") < ids.indexOf("console_counter"));
        QVERIFY(ids.indexOf("console_counter") < ids.indexOf("desktop"));
        QVERIFY(ids.indexOf("desktop") < ids.indexOf("desktop_text_editor"));
        QVERIFY(!ids.contains("desktop_empty"));
        QVERIFY(!ids.contains("desktop_mdi"));
        QVERIFY(!ids.contains("internal_console_graph"));
    }

    void hiddenDesktopTemplatesStayDiscoverableForInternalUse()
    {
        const auto templates = ProjectTemplates::availableTemplates(true);
        QStringList ids;
        for (const auto &tmpl : templates)
            ids.append(tmpl.id);

        QVERIFY(ids.contains("desktop_empty"));
        QVERIFY(ids.contains("desktop_mdi"));

        ProjectTemplateInfo emptyInfo;
        QVERIFY(ProjectTemplates::templateInfo("desktop_empty", &emptyInfo));
        QCOMPARE(emptyInfo.catalogRole, QString("internal_only"));
        QVERIFY(!emptyInfo.isUserVisible());
        QVERIFY(!emptyInfo.hiddenReason.isEmpty());

        ProjectTemplateInfo mdiInfo;
        QVERIFY(ProjectTemplates::templateInfo("desktop_mdi", &mdiInfo));
        QCOMPARE(mdiInfo.catalogRole, QString("internal_only"));
        QVERIFY(!mdiInfo.isUserVisible());
        QVERIFY(!mdiInfo.hiddenReason.isEmpty());
    }

    void desktopCatalogRolesAreExplicit()
    {
        ProjectTemplateInfo desktopInfo;
        QVERIFY(ProjectTemplates::templateInfo("desktop", &desktopInfo));
        QCOMPARE(desktopInfo.name, QString("Desktop UI Baseline"));
        QCOMPARE(desktopInfo.catalogRole, QString("advanced_desktop_template"));
        QVERIFY(desktopInfo.isUserVisible());
        QVERIFY(!desktopInfo.name.contains("Example", Qt::CaseInsensitive));
        QVERIFY(!desktopInfo.description.contains("Existing", Qt::CaseInsensitive));

        ProjectTemplateInfo textEditorInfo;
        QVERIFY(ProjectTemplates::templateInfo("desktop_text_editor", &textEditorInfo));
        QCOMPARE(textEditorInfo.catalogRole, QString("recommended_starter"));
        QVERIFY(textEditorInfo.isUserVisible());
        QVERIFY(textEditorInfo.description.contains("notes starter", Qt::CaseInsensitive));
        QVERIFY(!textEditorInfo.description.contains("text pad", Qt::CaseInsensitive));
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

        const QJsonObject layout = loadJsonObject(tmpDir.path() + "/UiProject/ui/window1.dqui");
        const QJsonObject window = layout.value("window").toObject();
        const QJsonObject properties = window.value("properties").toObject();
        QCOMPARE(properties.value("title").toString(), QString("Desktop UI Baseline"));
        QCOMPARE(properties.value("min_width").toInt(), 480);
        QCOMPARE(properties.value("min_height").toInt(), 360);
        QCOMPARE(properties.value("resizable").toBool(), true);

        const QJsonObject graph = loadJsonObject(tmpDir.path() + "/UiProject/graphs/main.dqgraph");
        const QJsonObject titleNode = findFirstNodeByModuleId(graph, "core.io.string_constant");
        QCOMPARE(titleNode.value("properties").toObject().value("value").toString(),
                 QString("\"Desktop UI Baseline\""));
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
        QCOMPARE(titleValue, QString("\"Workspace Notes\""));

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
        QVERIFY(layoutJson.contains("\"menuMain\""));
        QVERIFY(layoutJson.contains("\"tabsWorkspace\""));
        QVERIFY(layoutJson.contains("\"statusMain\""));
        QVERIFY(layoutJson.contains("\"lblStatus\""));
        QVERIFY(layoutJson.contains("\"MenuBar\""));
        QVERIFY(layoutJson.contains("\"TabPanel\""));
        QVERIFY(layoutJson.contains("\"StatusBar\""));

        const QJsonObject layout = loadJsonObject(tmpDir.path() + "/TextPadProject/ui/window1.dqui");
        const QJsonObject window = layout.value("window").toObject();
        const QJsonObject properties = window.value("properties").toObject();
        QCOMPARE(properties.value("title").toString(), QString("Workspace Notes"));
        QCOMPARE(properties.value("min_width").toInt(), 900);
        QCOMPARE(properties.value("min_height").toInt(), 620);
        QCOMPARE(properties.value("resizable").toBool(), true);

        const QJsonObject menuBar = findFirstChildByName(window, "menuMain");
        const QJsonObject toolBar = findFirstChildByName(window, "toolsMain");
        const QJsonObject tabsWorkspace = findFirstChildByName(window, "tabsWorkspace");
        const QJsonObject statusBar = findFirstChildByName(window, "statusMain");
        QVERIFY(!menuBar.isEmpty());
        QVERIFY(!toolBar.isEmpty());
        QVERIFY(!tabsWorkspace.isEmpty());
        QVERIFY(!statusBar.isEmpty());
        QCOMPARE(menuBar.value("type").toString(), QString("MenuBar"));
        QCOMPARE(toolBar.value("type").toString(), QString("ToolBar"));
        QCOMPARE(tabsWorkspace.value("type").toString(), QString("TabPanel"));
        QCOMPARE(statusBar.value("type").toString(), QString("StatusBar"));

        const QJsonObject docName = findFirstChildByName(tabsWorkspace, "lblDocName");
        const QJsonObject stats = findFirstChildByName(tabsWorkspace, "lblDocumentStats");
        const QJsonObject appendButton = findFirstChildByName(tabsWorkspace, "btnAppendLine");
        QVERIFY(!docName.isEmpty());
        QVERIFY(!stats.isEmpty());
        QVERIFY(!appendButton.isEmpty());
        QCOMPARE(docName.value("type").toString(), QString("Label"));
        QCOMPARE(stats.value("type").toString(), QString("Label"));
        QCOMPARE(appendButton.value("type").toString(), QString("Button"));

        QFile eventsFile(tmpDir.path() + "/TextPadProject/src/ui/window1_events.c");
        QVERIFY(eventsFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString eventsCode = QString::fromUtf8(eventsFile.readAll());
        QVERIFY(eventsCode.contains("DQ_DESKTOP_TEXT_EDITOR_AUTOCLOSE_MS"));
        QVERIFY(eventsCode.contains("Starter surfaces: menu, toolbar, tabs, status"));
        QVERIFY(!eventsCode.contains("UI Graph Example"));
    }
};

QTEST_MAIN(TestProjectTemplates)
#include "test_ProjectTemplates.moc"

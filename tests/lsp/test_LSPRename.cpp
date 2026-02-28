// Тесты LSP: TextEdit, WorkspaceEdit, definition[], references[], formatting
#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../../src/lsp/LSPTypes.h"

using namespace DeltaQ;

class TestLSPRename : public QObject {
    Q_OBJECT

private slots:
    // --- LSPTextEdit ---

    void testTextEditFromJson()
    {
        QJsonObject json;
        json["range"] = LSPRange{{5, 0}, {5, 10}}.toJson();
        json["newText"] = "newContent";

        auto edit = LSPTextEdit::fromJson(json);
        QCOMPARE(edit.range.start.line, 5);
        QCOMPARE(edit.range.start.character, 0);
        QCOMPARE(edit.range.end.line, 5);
        QCOMPARE(edit.range.end.character, 10);
        QCOMPARE(edit.newText, "newContent");
    }

    void testTextEditEmptyNewText()
    {
        QJsonObject json;
        json["range"] = LSPRange{{1, 0}, {1, 5}}.toJson();
        json["newText"] = "";

        auto edit = LSPTextEdit::fromJson(json);
        QVERIFY(edit.newText.isEmpty());
    }

    // --- WorkspaceEdit ---

    void testWorkspaceEditFromJson()
    {
        QJsonObject edit1;
        edit1["range"] = LSPRange{{0, 4}, {0, 8}}.toJson();
        edit1["newText"] = "newName";

        QJsonObject edit2;
        edit2["range"] = LSPRange{{10, 0}, {10, 4}}.toJson();
        edit2["newText"] = "newName";

        QJsonObject changes;
        changes["file:///tmp/test.cpp"] = QJsonArray({edit1, edit2});

        QJsonObject json;
        json["changes"] = changes;

        auto we = WorkspaceEdit::fromJson(json);
        QCOMPARE(we.changes.size(), 1);
        QVERIFY(we.changes.contains("file:///tmp/test.cpp"));
        QCOMPARE(we.changes["file:///tmp/test.cpp"].size(), 2);
        QCOMPARE(we.changes["file:///tmp/test.cpp"][0].newText, "newName");
        QCOMPARE(we.changes["file:///tmp/test.cpp"][1].range.start.line, 10);
    }

    void testWorkspaceEditEmpty()
    {
        QJsonObject json;
        json["changes"] = QJsonObject();

        auto we = WorkspaceEdit::fromJson(json);
        QVERIFY(we.changes.isEmpty());
    }

    void testWorkspaceEditMultipleFiles()
    {
        QJsonObject edit1;
        edit1["range"] = LSPRange{{0, 0}, {0, 3}}.toJson();
        edit1["newText"] = "foo";

        QJsonObject edit2;
        edit2["range"] = LSPRange{{5, 0}, {5, 3}}.toJson();
        edit2["newText"] = "foo";

        QJsonObject changes;
        changes["file:///a.cpp"] = QJsonArray({edit1});
        changes["file:///b.cpp"] = QJsonArray({edit2});

        QJsonObject json;
        json["changes"] = changes;

        auto we = WorkspaceEdit::fromJson(json);
        QCOMPARE(we.changes.size(), 2);
        QVERIFY(we.changes.contains("file:///a.cpp"));
        QVERIFY(we.changes.contains("file:///b.cpp"));
    }

    // --- Definition как массив Location[] ---

    void testDefinitionArray()
    {
        QJsonArray locations;
        QJsonObject loc1;
        loc1["uri"] = "file:///tmp/a.h";
        loc1["range"] = LSPRange{{10, 0}, {10, 5}}.toJson();
        locations.append(loc1);

        QJsonObject loc2;
        loc2["uri"] = "file:///tmp/b.h";
        loc2["range"] = LSPRange{{20, 0}, {20, 5}}.toJson();
        locations.append(loc2);

        // Парсим как массив Location[]
        QVector<LSPLocation> result;
        for (const auto &loc : locations)
            result.append(LSPLocation::fromJson(loc.toObject()));

        QCOMPARE(result.size(), 2);
        QCOMPARE(result[0].uri, "file:///tmp/a.h");
        QCOMPARE(result[0].range.start.line, 10);
        QCOMPARE(result[1].uri, "file:///tmp/b.h");
        QCOMPARE(result[1].range.start.line, 20);
    }

    // --- References как массив Location[] ---

    void testReferencesArray()
    {
        QJsonArray locations;
        for (int i = 0; i < 3; ++i) {
            QJsonObject loc;
            loc["uri"] = "file:///tmp/test.cpp";
            loc["range"] = LSPRange{{i * 10, 0}, {i * 10, 5}}.toJson();
            locations.append(loc);
        }

        QVector<LSPLocation> result;
        for (const auto &loc : locations)
            result.append(LSPLocation::fromJson(loc.toObject()));

        QCOMPARE(result.size(), 3);
        QCOMPARE(result[0].range.start.line, 0);
        QCOMPARE(result[1].range.start.line, 10);
        QCOMPARE(result[2].range.start.line, 20);
    }

    // --- Formatting: парсинг TextEdit[] ---

    void testFormattingParse()
    {
        QJsonArray edits;

        QJsonObject e1;
        e1["range"] = LSPRange{{0, 0}, {0, 4}}.toJson();
        e1["newText"] = "    ";
        edits.append(e1);

        QJsonObject e2;
        e2["range"] = LSPRange{{1, 0}, {1, 2}}.toJson();
        e2["newText"] = "  ";
        edits.append(e2);

        QVector<LSPTextEdit> result;
        for (const auto &e : edits)
            result.append(LSPTextEdit::fromJson(e.toObject()));

        QCOMPARE(result.size(), 2);
        QCOMPARE(result[0].newText, "    ");
        QCOMPARE(result[1].newText, "  ");
    }

    void testFormattingEmpty()
    {
        QJsonArray edits;
        QVector<LSPTextEdit> result;
        for (const auto &e : edits)
            result.append(LSPTextEdit::fromJson(e.toObject()));

        QVERIFY(result.isEmpty());
    }
};

QTEST_MAIN(TestLSPRename)
#include "test_LSPRename.moc"

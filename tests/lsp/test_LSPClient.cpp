// Тесты LSP-клиента — проверка JSON-RPC формирования и парсинга
#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSignalSpy>

#include "../../src/lsp/LSPClient.h"
#include "../../src/lsp/LSPTypes.h"

using namespace DeltaQ;

class TestLSPClient : public QObject {
    Q_OBJECT

private slots:
    // --- Тесты LSPTypes ---

    void testPosition()
    {
        LSPPosition pos{10, 5};
        auto json = pos.toJson();
        QCOMPARE(json["line"].toInt(), 10);
        QCOMPARE(json["character"].toInt(), 5);

        auto restored = LSPPosition::fromJson(json);
        QCOMPARE(restored.line, 10);
        QCOMPARE(restored.character, 5);
    }

    void testRange()
    {
        LSPRange range{{1, 0}, {1, 10}};
        auto json = range.toJson();

        auto restored = LSPRange::fromJson(json);
        QCOMPARE(restored.start.line, 1);
        QCOMPARE(restored.start.character, 0);
        QCOMPARE(restored.end.line, 1);
        QCOMPARE(restored.end.character, 10);
    }

    void testLocation()
    {
        QJsonObject json;
        json["uri"] = "file:///tmp/test.cpp";
        json["range"] = LSPRange{{5, 0}, {5, 10}}.toJson();

        auto loc = LSPLocation::fromJson(json);
        QCOMPARE(loc.uri, "file:///tmp/test.cpp");
        QCOMPARE(loc.range.start.line, 5);
        QCOMPARE(loc.range.end.character, 10);
    }

    void testDiagnostic()
    {
        QJsonObject json;
        json["range"] = LSPRange{{3, 0}, {3, 5}}.toJson();
        json["severity"] = 1; // Error
        json["message"] = "unknown type";
        json["source"] = "clangd";
        json["code"] = "err_unknown_type";

        auto diag = LSPDiagnostic::fromJson(json);
        QCOMPARE(diag.range.start.line, 3);
        QCOMPARE(diag.severity, DiagnosticSeverity::Error);
        QCOMPARE(diag.message, "unknown type");
        QCOMPARE(diag.source, "clangd");
    }

    void testDiagnosticWarning()
    {
        QJsonObject json;
        json["range"] = LSPRange{{10, 0}, {10, 8}}.toJson();
        json["severity"] = 2; // Warning
        json["message"] = "unused variable";

        auto diag = LSPDiagnostic::fromJson(json);
        QCOMPARE(diag.severity, DiagnosticSeverity::Warning);
    }

    void testCompletionItem()
    {
        QJsonObject json;
        json["label"] = "printf";
        json["kind"] = 3; // Function
        json["detail"] = "int printf(const char *, ...)";
        json["insertText"] = "printf";

        auto item = CompletionItem::fromJson(json);
        QCOMPARE(item.label, "printf");
        QCOMPARE(item.kind, CompletionItemKind::Function);
        QCOMPARE(item.detail, "int printf(const char *, ...)");
        QCOMPARE(item.insertText, "printf");
    }

    void testCompletionItemNoInsertText()
    {
        // Если insertText отсутствует — используется label
        QJsonObject json;
        json["label"] = "myFunc";
        json["kind"] = 3;

        auto item = CompletionItem::fromJson(json);
        QCOMPARE(item.insertText, "myFunc");
    }

    void testCompletionItemMarkupDoc()
    {
        // documentation как MarkupContent
        QJsonObject doc;
        doc["kind"] = "markdown";
        doc["value"] = "**Description**";

        QJsonObject json;
        json["label"] = "func";
        json["documentation"] = doc;

        auto item = CompletionItem::fromJson(json);
        QCOMPARE(item.documentation, "**Description**");
    }

    void testHoverInfo()
    {
        QJsonObject json;
        QJsonObject contents;
        contents["kind"] = "plaintext";
        contents["value"] = "int main()";
        json["contents"] = contents;
        json["range"] = LSPRange{{0, 4}, {0, 8}}.toJson();

        auto info = HoverInfo::fromJson(json);
        QCOMPARE(info.contents, "int main()");
        QCOMPARE(info.range.start.character, 4);
    }

    void testHoverInfoStringContents()
    {
        QJsonObject json;
        json["contents"] = QString("simple hover text");

        auto info = HoverInfo::fromJson(json);
        QCOMPARE(info.contents, "simple hover text");
    }

    void testDocumentSymbol()
    {
        QJsonObject json;
        json["name"] = "main";
        json["kind"] = 12; // Function
        json["range"] = LSPRange{{0, 0}, {10, 1}}.toJson();
        json["selectionRange"] = LSPRange{{0, 4}, {0, 8}}.toJson();

        auto sym = DocumentSymbol::fromJson(json);
        QCOMPARE(sym.name, "main");
        QCOMPARE(sym.kind, 12);
        QCOMPARE(sym.range.end.line, 10);
    }

    // --- Тесты утилит LSPClient ---

    void testPathToUri()
    {
        QString uri = LSPClient::pathToUri("/home/user/test.cpp");
        QCOMPARE(uri, "file:///home/user/test.cpp");
    }

    void testUriToPath()
    {
        QString path = LSPClient::uriToPath("file:///home/user/test.cpp");
        QCOMPARE(path, "/home/user/test.cpp");
    }

    void testPathUriRoundtrip()
    {
        QString original = "/tmp/project/src/main.cpp";
        QString restored = LSPClient::uriToPath(LSPClient::pathToUri(original));
        QCOMPARE(restored, original);
    }

    // --- Тест создания клиента ---

    void testClientNotRunning()
    {
        LSPClient client;
        QVERIFY(!client.isRunning());
    }

    void testClientStartNonexistent()
    {
        LSPClient client;
        QSignalSpy spy(&client, &LSPClient::serverError);
        client.start("/nonexistent/binary");
        QVERIFY(!client.isRunning());
        QVERIFY(spy.count() > 0);
    }
};

QTEST_MAIN(TestLSPClient)
#include "test_LSPClient.moc"

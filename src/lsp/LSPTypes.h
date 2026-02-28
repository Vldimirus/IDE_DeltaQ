// Типы данных протокола LSP (Language Server Protocol)
#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

namespace DeltaQ {

// Позиция в документе (0-based)
struct LSPPosition {
    int line = 0;
    int character = 0;

    QJsonObject toJson() const {
        return {{"line", line}, {"character", character}};
    }
    static LSPPosition fromJson(const QJsonObject &obj) {
        return {obj["line"].toInt(), obj["character"].toInt()};
    }
};

// Диапазон в документе
struct LSPRange {
    LSPPosition start;
    LSPPosition end;

    QJsonObject toJson() const {
        return {{"start", start.toJson()}, {"end", end.toJson()}};
    }
    static LSPRange fromJson(const QJsonObject &obj) {
        return {LSPPosition::fromJson(obj["start"].toObject()),
                LSPPosition::fromJson(obj["end"].toObject())};
    }
};

// Расположение символа (файл + диапазон)
struct LSPLocation {
    QString uri;
    LSPRange range;

    static LSPLocation fromJson(const QJsonObject &obj) {
        LSPLocation loc;
        loc.uri = obj["uri"].toString();
        loc.range = LSPRange::fromJson(obj["range"].toObject());
        return loc;
    }
};

// Идентификатор текстового документа
struct TextDocumentIdentifier {
    QString uri;

    QJsonObject toJson() const {
        return {{"uri", uri}};
    }
};

// Позиция в текстовом документе
struct TextDocumentPositionParams {
    TextDocumentIdentifier textDocument;
    LSPPosition position;

    QJsonObject toJson() const {
        return {{"textDocument", textDocument.toJson()},
                {"position", position.toJson()}};
    }
};

// Тяжесть диагностики
enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

// Диагностическое сообщение (ошибка/предупреждение)
struct LSPDiagnostic {
    LSPRange range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    QString code;
    QString source;
    QString message;

    static LSPDiagnostic fromJson(const QJsonObject &obj) {
        LSPDiagnostic d;
        d.range = LSPRange::fromJson(obj["range"].toObject());
        d.severity = static_cast<DiagnosticSeverity>(obj["severity"].toInt(1));
        d.code = obj["code"].toVariant().toString();
        d.source = obj["source"].toString();
        d.message = obj["message"].toString();
        return d;
    }
};

// Тип элемента автодополнения
enum class CompletionItemKind {
    Text = 1, Method = 2, Function = 3, Constructor = 4,
    Field = 5, Variable = 6, Class = 7, Interface = 8,
    Module = 9, Property = 10, Unit = 11, Value = 12,
    Enum = 13, Keyword = 14, Snippet = 15, Color = 16,
    File = 17, Reference = 18, Folder = 19, EnumMember = 20,
    Constant = 21, Struct = 22, Event = 23, Operator = 24,
    TypeParameter = 25
};

// Элемент автодополнения
struct CompletionItem {
    QString label;
    CompletionItemKind kind = CompletionItemKind::Text;
    QString detail;
    QString documentation;
    QString insertText;
    QString filterText;
    QString sortText;

    static CompletionItem fromJson(const QJsonObject &obj) {
        CompletionItem item;
        item.label = obj["label"].toString();
        item.kind = static_cast<CompletionItemKind>(obj["kind"].toInt(1));
        item.detail = obj["detail"].toString();

        // documentation может быть строкой или MarkupContent
        auto doc = obj["documentation"];
        if (doc.isString())
            item.documentation = doc.toString();
        else if (doc.isObject())
            item.documentation = doc.toObject()["value"].toString();

        item.insertText = obj["insertText"].toString();
        if (item.insertText.isEmpty())
            item.insertText = item.label;
        item.filterText = obj["filterText"].toString();
        item.sortText = obj["sortText"].toString();
        return item;
    }
};

// Информация при наведении (hover)
struct HoverInfo {
    QString contents; // Markdown
    LSPRange range;

    static HoverInfo fromJson(const QJsonObject &obj) {
        HoverInfo info;
        auto contents = obj["contents"];
        if (contents.isString()) {
            info.contents = contents.toString();
        } else if (contents.isObject()) {
            info.contents = contents.toObject()["value"].toString();
        } else if (contents.isArray()) {
            QStringList parts;
            for (const auto &item : contents.toArray()) {
                if (item.isString())
                    parts.append(item.toString());
                else if (item.isObject())
                    parts.append(item.toObject()["value"].toString());
            }
            info.contents = parts.join("\n");
        }
        if (obj.contains("range"))
            info.range = LSPRange::fromJson(obj["range"].toObject());
        return info;
    }
};

// Символ документа
struct DocumentSymbol {
    QString name;
    int kind = 0; // SymbolKind
    LSPRange range;
    LSPRange selectionRange;
    QVector<DocumentSymbol> children;

    static DocumentSymbol fromJson(const QJsonObject &obj) {
        DocumentSymbol sym;
        sym.name = obj["name"].toString();
        sym.kind = obj["kind"].toInt();
        sym.range = LSPRange::fromJson(obj["range"].toObject());
        sym.selectionRange = LSPRange::fromJson(obj["selectionRange"].toObject());
        if (obj.contains("children")) {
            for (const auto &child : obj["children"].toArray())
                sym.children.append(DocumentSymbol::fromJson(child.toObject()));
        }
        return sym;
    }
};

} // namespace DeltaQ

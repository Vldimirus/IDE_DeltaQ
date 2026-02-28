// LSP-клиент для взаимодействия с clangd
#include "LSPClient.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>
#include <QUrl>

namespace DeltaQ {

LSPClient::LSPClient(QObject *parent)
    : QObject(parent)
{
}

LSPClient::~LSPClient()
{
    stop();
}

// --- Управление процессом ---

void LSPClient::start(const QString &serverPath, const QStringList &args)
{
    if (m_process) {
        stop();
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &LSPClient::onReadyRead);
    connect(m_process, &QProcess::errorOccurred, this, &LSPClient::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::onProcessFinished);

    m_process->start(serverPath, args);
    if (!m_process->waitForStarted(5000)) {
        emit serverError(tr("Failed to start LSP server: %1").arg(serverPath));
        delete m_process;
        m_process = nullptr;
    }
}

void LSPClient::stop()
{
    if (!m_process)
        return;

    if (m_initialized) {
        shutdown();
        m_process->waitForFinished(3000);
    }

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(2000))
            m_process->kill();
    }

    delete m_process;
    m_process = nullptr;
    m_initialized = false;
    m_buffer.clear();
    m_pendingRequests.clear();
    m_nextId = 1;
}

bool LSPClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

// --- Инициализация LSP ---

void LSPClient::initialize(const QString &rootUri)
{
    QJsonObject capabilities;

    // Возможности текстового документа
    QJsonObject textDocumentSync;
    textDocumentSync["openClose"] = true;
    textDocumentSync["change"] = 1; // Full sync
    textDocumentSync["save"] = true;

    QJsonObject textDoc;
    textDoc["synchronization"] = textDocumentSync;

    // Автодополнение
    QJsonObject completionItem;
    completionItem["snippetSupport"] = false;
    QJsonObject completion;
    completion["completionItem"] = completionItem;
    textDoc["completion"] = completion;

    // Hover
    QJsonObject hover;
    hover["contentFormat"] = QJsonArray({"plaintext"});
    textDoc["hover"] = hover;

    // Definition
    textDoc["definition"] = QJsonObject({{"linkSupport", false}});

    // Diagnostics
    QJsonObject publishDiag;
    publishDiag["relatedInformation"] = false;
    textDoc["publishDiagnostics"] = publishDiag;

    capabilities["textDocument"] = textDoc;

    QJsonObject params;
    params["processId"] = static_cast<int>(QCoreApplication::applicationPid());
    params["rootUri"] = rootUri;
    params["capabilities"] = capabilities;

    sendRequest("initialize", params);
}

void LSPClient::shutdown()
{
    sendRequest("shutdown", QJsonObject());
    sendNotification("exit", QJsonObject());
    m_initialized = false;
}

// --- Синхронизация документов ---

void LSPClient::didOpen(const QString &uri, const QString &languageId, const QString &text)
{
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    textDoc["languageId"] = languageId;
    textDoc["version"] = 1;
    textDoc["text"] = text;

    QJsonObject params;
    params["textDocument"] = textDoc;
    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString &uri, int version, const QString &text)
{
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    textDoc["version"] = version;

    // Полная синхронизация (отправляем весь текст)
    QJsonObject change;
    change["text"] = text;

    QJsonObject params;
    params["textDocument"] = textDoc;
    params["contentChanges"] = QJsonArray({change});
    sendNotification("textDocument/didChange", params);
}

void LSPClient::didSave(const QString &uri)
{
    QJsonObject textDoc;
    textDoc["uri"] = uri;

    QJsonObject params;
    params["textDocument"] = textDoc;
    sendNotification("textDocument/didSave", params);
}

void LSPClient::didClose(const QString &uri)
{
    QJsonObject textDoc;
    textDoc["uri"] = uri;

    QJsonObject params;
    params["textDocument"] = textDoc;
    sendNotification("textDocument/didClose", params);
}

// --- Запросы ---

void LSPClient::completion(const QString &uri, int line, int character)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({{"line", line}, {"character", character}});
    sendRequest("textDocument/completion", params);
}

void LSPClient::hover(const QString &uri, int line, int character)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({{"line", line}, {"character", character}});
    sendRequest("textDocument/hover", params);
}

void LSPClient::definition(const QString &uri, int line, int character)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({{"line", line}, {"character", character}});
    sendRequest("textDocument/definition", params);
}

void LSPClient::references(const QString &uri, int line, int character)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({{"line", line}, {"character", character}});
    QJsonObject context;
    context["includeDeclaration"] = true;
    params["context"] = context;
    sendRequest("textDocument/references", params);
}

void LSPClient::rename(const QString &uri, int line, int character, const QString &newName)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    params["position"] = QJsonObject({{"line", line}, {"character", character}});
    params["newName"] = newName;
    sendRequest("textDocument/rename", params);
}

void LSPClient::formatting(const QString &uri, int tabSize, bool insertSpaces)
{
    QJsonObject params;
    params["textDocument"] = QJsonObject({{"uri", uri}});
    QJsonObject options;
    options["tabSize"] = tabSize;
    options["insertSpaces"] = insertSpaces;
    params["options"] = options;
    sendRequest("textDocument/formatting", params);
}

// --- Утилиты ---

QString LSPClient::pathToUri(const QString &path)
{
    return QUrl::fromLocalFile(path).toString();
}

QString LSPClient::uriToPath(const QString &uri)
{
    return QUrl(uri).toLocalFile();
}

// --- JSON-RPC протокол ---

int LSPClient::sendRequest(const QString &method, const QJsonObject &params)
{
    int id = m_nextId++;
    m_pendingRequests[id] = method;

    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = id;
    msg["method"] = method;
    if (!params.isEmpty())
        msg["params"] = params;

    sendMessage(msg);
    return id;
}

void LSPClient::sendNotification(const QString &method, const QJsonObject &params)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = method;
    if (!params.isEmpty())
        msg["params"] = params;

    sendMessage(msg);
}

void LSPClient::sendMessage(const QJsonObject &message)
{
    if (!m_process || m_process->state() != QProcess::Running)
        return;

    QByteArray content = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray header = "Content-Length: " + QByteArray::number(content.size()) + "\r\n\r\n";

    m_process->write(header + content);
}

// --- Обработка входящих данных ---

void LSPClient::onReadyRead()
{
    m_buffer.append(m_process->readAllStandardOutput());
    processIncomingData();
}

void LSPClient::processIncomingData()
{
    // Парсим сообщения формата Content-Length: N\r\n\r\n{json}
    while (true) {
        // Ищем конец заголовков
        int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            break;

        // Парсим Content-Length
        QByteArray header = m_buffer.left(headerEnd);
        int contentLength = -1;

        for (const auto &line : header.split('\n')) {
            QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:")) {
                contentLength = trimmed.mid(15).trimmed().toInt();
                break;
            }
        }

        if (contentLength < 0) {
            // Некорректный заголовок — пропускаем
            m_buffer.remove(0, headerEnd + 4);
            continue;
        }

        int messageStart = headerEnd + 4;
        int totalSize = messageStart + contentLength;

        // Ждём полное сообщение
        if (m_buffer.size() < totalSize)
            break;

        QByteArray content = m_buffer.mid(messageStart, contentLength);
        m_buffer.remove(0, totalSize);

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(content, &parseError);
        if (parseError.error != QJsonParseError::NoError)
            continue;

        handleMessage(doc.object());
    }
}

void LSPClient::handleMessage(const QJsonObject &message)
{
    if (message.contains("id") && message.contains("method")) {
        // Запрос от сервера (server → client request) — пока игнорируем
        return;
    }

    if (message.contains("id")) {
        // Ответ на наш запрос
        int id = message["id"].toInt();
        QJsonValue result = message["result"];
        QJsonObject error = message["error"].toObject();
        handleResponse(id, result, error);
        return;
    }

    if (message.contains("method")) {
        // Нотификация от сервера
        QString method = message["method"].toString();
        QJsonObject params = message["params"].toObject();
        handleNotification(method, params);
    }
}

void LSPClient::handleResponse(int id, const QJsonValue &result, const QJsonObject &error)
{
    QString method = m_pendingRequests.take(id);
    if (method.isEmpty())
        return;

    // Ошибка
    if (!error.isEmpty()) {
        emit serverError(error["message"].toString());
        return;
    }

    if (method == "initialize") {
        // Отправляем initialized
        sendNotification("initialized", QJsonObject());
        m_initialized = true;
        emit initialized();
    }
    else if (method == "textDocument/completion") {
        QVector<CompletionItem> items;
        QJsonArray itemsArray;

        if (result.isArray()) {
            // Ответ — массив CompletionItem[]
            itemsArray = result.toArray();
        } else if (result.isObject()) {
            // Ответ — CompletionList { items: [...] }
            QJsonObject obj = result.toObject();
            itemsArray = obj["items"].toArray();
        }

        for (const auto &item : itemsArray)
            items.append(CompletionItem::fromJson(item.toObject()));

        emit completionResult(items);
    }
    else if (method == "textDocument/hover") {
        if (result.isObject()) {
            emit hoverResult(HoverInfo::fromJson(result.toObject()));
        }
    }
    else if (method == "textDocument/definition") {
        QVector<LSPLocation> locations;
        if (result.isArray()) {
            // Location[]
            for (const auto &loc : result.toArray())
                locations.append(LSPLocation::fromJson(loc.toObject()));
        } else if (result.isObject()) {
            // Одиночный Location
            locations.append(LSPLocation::fromJson(result.toObject()));
        }
        emit definitionResult(locations);
    }
    else if (method == "textDocument/references") {
        QVector<LSPLocation> locations;
        if (result.isArray()) {
            for (const auto &loc : result.toArray())
                locations.append(LSPLocation::fromJson(loc.toObject()));
        } else if (result.isObject()) {
            locations.append(LSPLocation::fromJson(result.toObject()));
        }
        emit referencesResult(locations);
    }
    else if (method == "textDocument/rename") {
        if (result.isObject()) {
            emit renameResult(WorkspaceEdit::fromJson(result.toObject()));
        }
    }
    else if (method == "textDocument/formatting") {
        QVector<LSPTextEdit> edits;
        if (result.isArray()) {
            for (const auto &e : result.toArray())
                edits.append(LSPTextEdit::fromJson(e.toObject()));
        }
        emit formattingResult(edits);
    }
}

void LSPClient::handleNotification(const QString &method, const QJsonObject &params)
{
    if (method == "textDocument/publishDiagnostics") {
        QString uri = params["uri"].toString();
        QVector<LSPDiagnostic> diagnostics;
        for (const auto &diag : params["diagnostics"].toArray())
            diagnostics.append(LSPDiagnostic::fromJson(diag.toObject()));
        emit diagnosticsReceived(uri, diagnostics);
    }
    // Другие нотификации (window/logMessage и т.д.) — пока игнорируем
}

void LSPClient::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    if (m_process)
        emit serverError(tr("LSP server error: %1").arg(m_process->errorString()));
}

void LSPClient::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(status)
    m_initialized = false;
    emit serverStopped();
}

} // namespace DeltaQ

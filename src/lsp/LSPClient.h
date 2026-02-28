// LSP-клиент для взаимодействия с clangd
#pragma once

#include "LSPTypes.h"

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QMap>
#include <functional>

namespace DeltaQ {

class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject *parent = nullptr);
    ~LSPClient() override;

    // Управление процессом
    void start(const QString &serverPath, const QStringList &args = {});
    void stop();
    bool isRunning() const;

    // Инициализация LSP
    void initialize(const QString &rootUri);
    void shutdown();

    // Синхронизация документов
    void didOpen(const QString &uri, const QString &languageId, const QString &text);
    void didChange(const QString &uri, int version, const QString &text);
    void didSave(const QString &uri);
    void didClose(const QString &uri);

    // Запросы
    void completion(const QString &uri, int line, int character);
    void hover(const QString &uri, int line, int character);
    void definition(const QString &uri, int line, int character);
    void references(const QString &uri, int line, int character);
    void rename(const QString &uri, int line, int character, const QString &newName);
    void formatting(const QString &uri, int tabSize = 4, bool insertSpaces = true);

    // Утилиты
    static QString pathToUri(const QString &path);
    static QString uriToPath(const QString &uri);

signals:
    // Состояние
    void initialized();
    void serverError(const QString &message);
    void serverStopped();

    // Диагностика
    void diagnosticsReceived(const QString &uri, const QVector<LSPDiagnostic> &diagnostics);

    // Ответы на запросы
    void completionResult(const QVector<CompletionItem> &items);
    void hoverResult(const HoverInfo &info);
    void definitionResult(const QVector<LSPLocation> &locations);
    void referencesResult(const QVector<LSPLocation> &locations);
    void renameResult(const WorkspaceEdit &edits);
    void formattingResult(const QVector<LSPTextEdit> &edits);

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    // JSON-RPC протокол
    int sendRequest(const QString &method, const QJsonObject &params);
    void sendNotification(const QString &method, const QJsonObject &params);
    void sendMessage(const QJsonObject &message);
    void processIncomingData();
    void handleMessage(const QJsonObject &message);
    void handleResponse(int id, const QJsonValue &result, const QJsonObject &error);
    void handleNotification(const QString &method, const QJsonObject &params);

    QProcess *m_process = nullptr;
    QByteArray m_buffer; // буфер входящих данных
    int m_nextId = 1;
    bool m_initialized = false;

    // Ожидающие запросы: id → method
    QMap<int, QString> m_pendingRequests;
};

} // namespace DeltaQ

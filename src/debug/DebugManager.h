// Менеджер отладки — интерфейс к GDB через MI протокол
#pragma once

#include <QObject>
#include <QProcess>
#include <QMap>
#include "DebugTypes.h"

namespace DeltaQ {

class DebugManager : public QObject {
    Q_OBJECT

public:
    explicit DebugManager(QObject *parent = nullptr);
    ~DebugManager() override;

    // Управление
    void startDebug(const QString &executable, const QStringList &args = {});
    void stopDebug();
    void pauseExecution();
    void continueExecution();

    // Шаги
    void stepOver();
    void stepInto();
    void stepOut();
    void runToCursor(const QString &file, int line);

    // Точки останова
    void addBreakpoint(const QString &file, int line);
    void removeBreakpoint(const QString &file, int line);
    void toggleBreakpoint(const QString &file, int line);
    void setConditionalBreakpoint(const QString &file, int line, const QString &condition);
    QVector<Breakpoint> breakpoints() const { return m_breakpoints; }

    // Переменные
    void evaluateExpression(const QString &expression);
    QVector<Variable> localVariables() const { return m_localVariables; }
    void requestLocalVariables();

    // Стек вызовов
    QVector<StackFrame> callStack() const { return m_callStack; }
    void requestCallStack();
    void selectFrame(int index);

    // Состояние
    DebugState state() const { return m_state; }
    bool isRunning() const { return m_state == DebugState::Running || m_state == DebugState::Paused; }

    // Парсинг MI-вывода (public для тестирования)
    struct MIRecord {
        enum Type { Result, ExecAsync, StatusAsync, NotifyAsync, ConsoleStream, TargetStream, LogStream };
        Type type = Result;
        int token = -1;
        QString resultClass;   // "done", "running", "error", "stopped", ...
        QString payload;       // Необработанная полезная нагрузка
    };

    static MIRecord parseMIOutput(const QString &line);
    static QMap<QString, QString> parseMITuple(const QString &tuple);
    static QVector<QMap<QString, QString>> parseMIList(const QString &list);
    static QString unquoteMI(const QString &str);

signals:
    void debugStarted();
    void debugStopped();
    void stateChanged(DebugState state);
    void breakpointHit(const QString &file, int line);
    void breakpointAdded(const Breakpoint &bp);
    void breakpointRemoved(const QString &file, int line);
    void stepped(const QString &file, int line);
    void variablesUpdated(const QVector<Variable> &locals);
    void callStackUpdated(const QVector<StackFrame> &frames);
    void expressionEvaluated(const QString &expr, const QString &value);
    void debugOutput(const QString &text);
    void targetOutput(const QString &text);
    void errorOccurred(const QString &message);

private slots:
    void onProcessOutput();
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void sendCommand(const QString &command);
    void processLine(const QString &line);
    void handleResultRecord(const MIRecord &record);
    void handleExecAsyncRecord(const MIRecord &record);
    void setState(DebugState state);

    QProcess *m_process = nullptr;
    DebugState m_state = DebugState::Idle;
    int m_nextToken = 1;
    QMap<int, QString> m_pendingCommands; // token -> команда
    QVector<Breakpoint> m_breakpoints;
    QVector<Variable> m_localVariables;
    QVector<StackFrame> m_callStack;
    QString m_buffer; // Буфер для неполных строк
};

} // namespace DeltaQ

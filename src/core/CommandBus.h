#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <memory>
#include <functional>
#include <vector>

namespace DeltaQ {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual QString description() const = 0;
    virtual bool mergeWith(const Command *) { return false; }
};

using CommandPtr = std::unique_ptr<Command>;

class LambdaCommand : public Command {
public:
    LambdaCommand(QString desc, std::function<void()> exec, std::function<void()> undo)
        : m_desc(std::move(desc)), m_exec(std::move(exec)), m_undo(std::move(undo)) {}

    void execute() override { m_exec(); }
    void undo() override { m_undo(); }
    QString description() const override { return m_desc; }

private:
    QString m_desc;
    std::function<void()> m_exec;
    std::function<void()> m_undo;
};

// Макрокоманда — группирует несколько команд в одну транзакцию
class MacroCommand : public Command {
public:
    explicit MacroCommand(QString desc) : m_desc(std::move(desc)) {}

    void execute() override {
        for (auto &cmd : m_commands)
            cmd->execute();
    }

    void undo() override {
        // Отмена в обратном порядке
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it)
            (*it)->undo();
    }

    QString description() const override { return m_desc; }

    void addCommand(CommandPtr cmd) {
        m_commands.push_back(std::move(cmd));
    }

    bool isEmpty() const { return m_commands.empty(); }
    int count() const { return static_cast<int>(m_commands.size()); }

private:
    QString m_desc;
    std::vector<CommandPtr> m_commands;
};

class CommandBus : public QObject {
    Q_OBJECT

public:
    explicit CommandBus(QObject *parent = nullptr);

    void execute(CommandPtr cmd);
    void executeNoHistory(CommandPtr cmd);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    void clear();

    // Макрокоманды
    void beginMacro(const QString &description);
    void endMacro();
    bool isInMacro() const;

signals:
    void commandExecuted(const QString &description);
    void undoPerformed(const QString &description);
    void redoPerformed(const QString &description);
    void stateChanged();

private:
    std::vector<CommandPtr> m_undoStack;
    std::vector<CommandPtr> m_redoStack;
    static constexpr int MaxUndoDepth = 100;

    mutable QMutex m_mutex;
    std::unique_ptr<MacroCommand> m_activeMacro;
    int m_macroDepth = 0;
};

} // namespace DeltaQ

#pragma once

#include <QObject>
#include <QString>
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

class CommandBus : public QObject {
    Q_OBJECT

public:
    explicit CommandBus(QObject *parent = nullptr);

    void execute(CommandPtr cmd);
    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    void clear();

signals:
    void commandExecuted(const QString &description);
    void undoPerformed(const QString &description);
    void redoPerformed(const QString &description);
    void stateChanged();

private:
    std::vector<CommandPtr> m_undoStack;
    std::vector<CommandPtr> m_redoStack;
    static constexpr int MaxUndoDepth = 100;
};

} // namespace DeltaQ

// CommandBus — шина команд с undo/redo
#include "CommandBus.h"

namespace DeltaQ {

CommandBus::CommandBus(QObject *parent)
    : QObject(parent)
{
}

void CommandBus::execute(CommandPtr cmd)
{
    cmd->execute();
    m_redoStack.clear();

    // Ограничение глубины стека отмены
    if (static_cast<int>(m_undoStack.size()) >= MaxUndoDepth)
        m_undoStack.erase(m_undoStack.begin());

    QString desc = cmd->description();
    m_undoStack.push_back(std::move(cmd));

    emit commandExecuted(desc);
    emit stateChanged();
}

void CommandBus::undo()
{
    if (m_undoStack.empty())
        return;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    cmd->undo();
    QString desc = cmd->description();
    m_redoStack.push_back(std::move(cmd));

    emit undoPerformed(desc);
    emit stateChanged();
}

void CommandBus::redo()
{
    if (m_redoStack.empty())
        return;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    cmd->execute();
    QString desc = cmd->description();
    m_undoStack.push_back(std::move(cmd));

    emit redoPerformed(desc);
    emit stateChanged();
}

bool CommandBus::canUndo() const { return !m_undoStack.empty(); }
bool CommandBus::canRedo() const { return !m_redoStack.empty(); }

QString CommandBus::undoText() const
{
    return m_undoStack.empty() ? QString() : m_undoStack.back()->description();
}

QString CommandBus::redoText() const
{
    return m_redoStack.empty() ? QString() : m_redoStack.back()->description();
}

void CommandBus::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
    emit stateChanged();
}

} // namespace DeltaQ

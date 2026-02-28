// CommandBus — шина команд с undo/redo, потокобезопасность, макрокоманды
#include "CommandBus.h"
#include <QMutexLocker>

namespace DeltaQ {

CommandBus::CommandBus(QObject *parent)
    : QObject(parent)
{
}

void CommandBus::execute(CommandPtr cmd)
{
    QString desc;
    {
        QMutexLocker locker(&m_mutex);

        // Если активна макрокоманда — добавить в неё
        if (m_activeMacro) {
            cmd->execute();
            m_activeMacro->addCommand(std::move(cmd));
            return;
        }

        cmd->execute();
        m_redoStack.clear();

        // Ограничение глубины стека отмены
        if (static_cast<int>(m_undoStack.size()) >= MaxUndoDepth)
            m_undoStack.erase(m_undoStack.begin());

        desc = cmd->description();
        m_undoStack.push_back(std::move(cmd));
    }
    // Сигналы вне мьютекса — избежать deadlock
    emit commandExecuted(desc);
    emit stateChanged();
}

void CommandBus::executeNoHistory(CommandPtr cmd)
{
    cmd->execute();
}

void CommandBus::undo()
{
    QString desc;
    {
        QMutexLocker locker(&m_mutex);

        if (m_undoStack.empty())
            return;

        auto cmd = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        cmd->undo();
        desc = cmd->description();
        m_redoStack.push_back(std::move(cmd));
    }
    emit undoPerformed(desc);
    emit stateChanged();
}

void CommandBus::redo()
{
    QString desc;
    {
        QMutexLocker locker(&m_mutex);

        if (m_redoStack.empty())
            return;

        auto cmd = std::move(m_redoStack.back());
        m_redoStack.pop_back();

        cmd->execute();
        desc = cmd->description();
        m_undoStack.push_back(std::move(cmd));
    }
    emit redoPerformed(desc);
    emit stateChanged();
}

bool CommandBus::canUndo() const
{
    QMutexLocker locker(&m_mutex);
    return !m_undoStack.empty();
}

bool CommandBus::canRedo() const
{
    QMutexLocker locker(&m_mutex);
    return !m_redoStack.empty();
}

QString CommandBus::undoText() const
{
    QMutexLocker locker(&m_mutex);
    return m_undoStack.empty() ? QString() : m_undoStack.back()->description();
}

QString CommandBus::redoText() const
{
    QMutexLocker locker(&m_mutex);
    return m_redoStack.empty() ? QString() : m_redoStack.back()->description();
}

void CommandBus::clear()
{
    {
        QMutexLocker locker(&m_mutex);
        m_undoStack.clear();
        m_redoStack.clear();
    }
    emit stateChanged();
}

void CommandBus::beginMacro(const QString &description)
{
    QMutexLocker locker(&m_mutex);
    if (m_macroDepth == 0) {
        m_activeMacro = std::make_unique<MacroCommand>(description);
    }
    ++m_macroDepth;
}

void CommandBus::endMacro()
{
    QString desc;
    bool shouldEmit = false;
    {
        QMutexLocker locker(&m_mutex);

        if (m_macroDepth <= 0)
            return;

        --m_macroDepth;
        if (m_macroDepth == 0 && m_activeMacro) {
            if (!m_activeMacro->isEmpty()) {
                m_redoStack.clear();
                if (static_cast<int>(m_undoStack.size()) >= MaxUndoDepth)
                    m_undoStack.erase(m_undoStack.begin());

                desc = m_activeMacro->description();
                m_undoStack.push_back(std::move(m_activeMacro));
                shouldEmit = true;
            }
            m_activeMacro.reset();
        }
    }
    if (shouldEmit) {
        emit commandExecuted(desc);
        emit stateChanged();
    }
}

bool CommandBus::isInMacro() const
{
    QMutexLocker locker(&m_mutex);
    return m_macroDepth > 0;
}

} // namespace DeltaQ

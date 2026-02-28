#include "UndoManager.h"
#include "CommandBus.h"

namespace DeltaQ {

UndoManager::UndoManager(CommandBus *bus, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
{
    connect(m_bus, &CommandBus::stateChanged, this, &UndoManager::stateChanged);

    // Отслеживание индекса для clean state
    connect(m_bus, &CommandBus::commandExecuted, this, [this]() {
        ++m_currentIndex;
        checkCleanState();
    });
    connect(m_bus, &CommandBus::undoPerformed, this, [this]() {
        --m_currentIndex;
        checkCleanState();
    });
    connect(m_bus, &CommandBus::redoPerformed, this, [this]() {
        ++m_currentIndex;
        checkCleanState();
    });
}

bool UndoManager::canUndo() const { return m_bus->canUndo(); }
bool UndoManager::canRedo() const { return m_bus->canRedo(); }
QString UndoManager::undoText() const { return m_bus->undoText(); }
QString UndoManager::redoText() const { return m_bus->redoText(); }

QString UndoManager::undoDescription() const { return m_bus->undoText(); }
QString UndoManager::redoDescription() const { return m_bus->redoText(); }

void UndoManager::undo() { m_bus->undo(); }
void UndoManager::redo() { m_bus->redo(); }

void UndoManager::setClean()
{
    m_cleanIndex = m_currentIndex;
    emit cleanChanged(true);
}

bool UndoManager::isClean() const
{
    return m_currentIndex == m_cleanIndex;
}

void UndoManager::clear()
{
    m_bus->clear();
    m_cleanIndex = -1;
    m_currentIndex = 0;
    checkCleanState();
}

void UndoManager::checkCleanState()
{
    // Сравниваем новое состояние с предыдущим для оптимизации сигналов
    bool clean = (m_currentIndex == m_cleanIndex);
    emit cleanChanged(clean);
}

} // namespace DeltaQ

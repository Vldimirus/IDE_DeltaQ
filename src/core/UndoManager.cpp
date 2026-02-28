#include "UndoManager.h"
#include "CommandBus.h"

namespace DeltaQ {

UndoManager::UndoManager(CommandBus *bus, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
{
    connect(m_bus, &CommandBus::stateChanged, this, &UndoManager::stateChanged);
}

bool UndoManager::canUndo() const { return m_bus->canUndo(); }
bool UndoManager::canRedo() const { return m_bus->canRedo(); }
QString UndoManager::undoText() const { return m_bus->undoText(); }
QString UndoManager::redoText() const { return m_bus->redoText(); }

void UndoManager::undo() { m_bus->undo(); }
void UndoManager::redo() { m_bus->redo(); }

} // namespace DeltaQ

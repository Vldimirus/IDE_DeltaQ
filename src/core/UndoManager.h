#pragma once

#include <QObject>

namespace DeltaQ {

class CommandBus;

// UndoManager is a thin wrapper that connects CommandBus to UI elements.
// The actual undo/redo logic lives in CommandBus.
class UndoManager : public QObject {
    Q_OBJECT

public:
    explicit UndoManager(CommandBus *bus, QObject *parent = nullptr);

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

public slots:
    void undo();
    void redo();

signals:
    void stateChanged();

private:
    CommandBus *m_bus;
};

} // namespace DeltaQ

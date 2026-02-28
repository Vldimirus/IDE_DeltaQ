#pragma once

#include <QObject>

namespace DeltaQ {

class CommandBus;

// UndoManager — обёртка над CommandBus для UI.
// Отслеживает «чистое» состояние (clean state) для индикации несохранённых изменений.
class UndoManager : public QObject {
    Q_OBJECT

public:
    explicit UndoManager(CommandBus *bus, QObject *parent = nullptr);

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;

    // Алиасы для UI (удобство именования)
    QString undoDescription() const;
    QString redoDescription() const;

    // Clean state — отслеживание несохранённых изменений
    void setClean();
    bool isClean() const;
    void clear();

public slots:
    void undo();
    void redo();

signals:
    void stateChanged();
    void cleanChanged(bool isClean);

private:
    void checkCleanState();

    CommandBus *m_bus;
    int m_cleanIndex = 0;
    int m_currentIndex = 0;
};

} // namespace DeltaQ

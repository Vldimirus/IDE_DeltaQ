#include "ActionManager.h"

namespace DeltaQ {

ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
}

QAction *ActionManager::registerAction(const QString &id, const QString &text,
                                        const QKeySequence &shortcut)
{
    auto *act = new QAction(text, this);
    if (!shortcut.isEmpty())
        act->setShortcut(shortcut);
    m_actions[id] = act;
    return act;
}

QAction *ActionManager::action(const QString &id) const
{
    return m_actions.value(id, nullptr);
}

QList<QAction *> ActionManager::allActions() const
{
    return m_actions.values();
}

void ActionManager::setupStandardActions()
{
    registerAction("file.newProject", tr("New Project..."), QKeySequence("Ctrl+Shift+N"));
    registerAction("file.openProject", tr("Open Project..."), QKeySequence::Open);
    registerAction("file.save", tr("Save"), QKeySequence::Save);
    registerAction("file.saveAll", tr("Save All"), QKeySequence("Ctrl+Shift+S"));
    registerAction("file.close", tr("Close"), QKeySequence::Close);
    registerAction("file.quit", tr("Quit"), QKeySequence::Quit);

    registerAction("edit.undo", tr("Undo"), QKeySequence::Undo);
    registerAction("edit.redo", tr("Redo"), QKeySequence::Redo);
    registerAction("edit.cut", tr("Cut"), QKeySequence::Cut);
    registerAction("edit.copy", tr("Copy"), QKeySequence::Copy);
    registerAction("edit.paste", tr("Paste"), QKeySequence::Paste);
    registerAction("edit.find", tr("Find..."), QKeySequence::Find);

    registerAction("build.build", tr("Build"), QKeySequence("Ctrl+B"));
    registerAction("build.run", tr("Run"), QKeySequence("Ctrl+R"));
    registerAction("build.clean", tr("Clean"), QKeySequence("Ctrl+Shift+B"));

    registerAction("view.codeEditor", tr("Code Editor"), QKeySequence("Ctrl+1"));
    registerAction("view.blockEditor", tr("Block Editor"), QKeySequence("Ctrl+2"));
    registerAction("view.uiDesigner", tr("UI Designer"), QKeySequence("Ctrl+3"));
    registerAction("view.libProcessor", tr("Library Processor"), QKeySequence("Ctrl+4"));
}

} // namespace DeltaQ

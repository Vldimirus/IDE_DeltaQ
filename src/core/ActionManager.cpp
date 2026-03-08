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

QAction *ActionManager::registerAction(const QString &id, const QString &text,
                                        const QKeySequence &shortcut,
                                        const QString &group)
{
    auto *act = registerAction(id, text, shortcut);
    addToGroup(id, group);
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

void ActionManager::addToGroup(const QString &actionId, const QString &groupId)
{
    m_groups[groupId].insert(actionId);
}

void ActionManager::enableGroup(const QString &groupId)
{
    const auto &ids = m_groups.value(groupId);
    for (const auto &id : ids) {
        if (auto *act = action(id))
            act->setEnabled(true);
    }
}

void ActionManager::disableGroup(const QString &groupId)
{
    const auto &ids = m_groups.value(groupId);
    for (const auto &id : ids) {
        if (auto *act = action(id))
            act->setEnabled(false);
    }
}

QList<QAction *> ActionManager::actionsInGroup(const QString &groupId) const
{
    QList<QAction *> result;
    const auto &ids = m_groups.value(groupId);
    for (const auto &id : ids) {
        if (auto *act = action(id))
            result.append(act);
    }
    return result;
}

void ActionManager::setupStandardActions()
{
    // Группа "file"
    registerAction("file.newProject", tr("New Project..."), QKeySequence("Ctrl+Shift+N"), "file");
    registerAction("file.openProject", tr("Open Project..."), QKeySequence::Open, "file");
    registerAction("file.save", tr("Save"), QKeySequence::Save, "file");
    registerAction("file.saveAll", tr("Save All"), QKeySequence("Ctrl+Shift+S"), "file");
    registerAction("file.close", tr("Close"), QKeySequence::Close, "file");
    registerAction("file.quit", tr("Quit"), QKeySequence::Quit, "file");

    // Группа "edit"
    registerAction("edit.undo", tr("Undo"), QKeySequence::Undo, "edit");
    registerAction("edit.redo", tr("Redo"), QKeySequence::Redo, "edit");
    registerAction("edit.cut", tr("Cut"), QKeySequence::Cut, "edit");
    registerAction("edit.copy", tr("Copy"), QKeySequence::Copy, "edit");
    registerAction("edit.paste", tr("Paste"), QKeySequence::Paste, "edit");
    registerAction("edit.find", tr("Find..."), QKeySequence::Find, "edit");
    registerAction("edit.replace", tr("Replace..."), QKeySequence::Replace, "edit");
    registerAction("edit.goToLine", tr("Go to Line..."), QKeySequence("Ctrl+G"), "edit");
    registerAction("edit.goToDefinition", tr("Go to Definition"), QKeySequence("F12"), "edit");
    registerAction("edit.findReferences", tr("Find References"), QKeySequence("Shift+F12"), "edit");
    registerAction("edit.openGeneratedOrigin", tr("Open Generated Origin"), {}, "edit");
    registerAction("edit.rename", tr("Rename Symbol"), QKeySequence("F2"), "edit");
    registerAction("edit.format", tr("Format Document"), QKeySequence("Ctrl+Shift+I"), "edit");

    // Группа "build"
    registerAction("build.build", tr("Build"), QKeySequence("Ctrl+B"), "build");
    registerAction("build.run", tr("Run"), QKeySequence("Ctrl+R"), "build");
    registerAction("build.clean", tr("Clean"), QKeySequence("Ctrl+Shift+B"), "build");

    // Группа "debug"
    registerAction("debug.start", tr("Start Debugging"), QKeySequence("F5"), "debug");
    registerAction("debug.stop", tr("Stop Debugging"), QKeySequence("Shift+F5"), "debug");
    registerAction("debug.continue", tr("Continue"), QKeySequence("F5"), "debug");
    registerAction("debug.stepOver", tr("Step Over"), QKeySequence("F10"), "debug");
    registerAction("debug.stepInto", tr("Step Into"), QKeySequence("F11"), "debug");
    registerAction("debug.stepOut", tr("Step Out"), QKeySequence("Shift+F11"), "debug");
    registerAction("debug.toggleBreakpoint", tr("Toggle Breakpoint"), QKeySequence("F9"), "debug");

    // Группа "view"
    registerAction("view.codeEditor", tr("Code Editor"), QKeySequence("Ctrl+1"), "view");
    registerAction("view.blockEditor", tr("Block Editor"), QKeySequence("Ctrl+2"), "view");
    registerAction("view.uiDesigner", tr("UI Designer"), QKeySequence("Ctrl+3"), "view");
    registerAction("view.libProcessor", tr("Library Processor"), QKeySequence("Ctrl+4"), "view");
}

} // namespace DeltaQ

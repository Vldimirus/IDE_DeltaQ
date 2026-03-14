#include "ActionManager.h"

#include <QApplication>
#include <QStyle>

void ensureToolbarResourcesLoaded()
{
    static bool loaded = []() {
        Q_INIT_RESOURCE(toolbar_icons);
        return true;
    }();
    Q_UNUSED(loaded);
}

namespace DeltaQ {

namespace {

QIcon presentationIcon(QStyle::StandardPixmap fallbackType, const QString &resourcePath = {})
{
    if (!resourcePath.isEmpty()) {
        QIcon icon(resourcePath);
        if (!icon.isNull())
            return icon;
    }

    if (QStyle *style = QApplication::style())
        return style->standardIcon(fallbackType);
    return {};
}

void setActionPresentation(QAction *action,
                           QStyle::StandardPixmap iconType,
                           const QString &resourcePath,
                           const QString &toolTip,
                           const QString &statusTip)
{
    if (!action)
        return;

    action->setIcon(presentationIcon(iconType, resourcePath));
    action->setToolTip(toolTip);
    action->setStatusTip(statusTip);
}

} // namespace

ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
    ensureToolbarResourcesLoaded();
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
    setActionPresentation(
        registerAction("file.newProject", tr("New Project..."), QKeySequence("Ctrl+Shift+N"), "file"),
        QStyle::SP_FileIcon,
        ":/icons/toolbar/new_project.svg",
        tr("Create a new DeltaQ project"),
        tr("Create a new DeltaQ project"));
    setActionPresentation(
        registerAction("file.openProject", tr("Open Project..."), QKeySequence::Open, "file"),
        QStyle::SP_DirOpenIcon,
        ":/icons/toolbar/open_project.svg",
        tr("Open an existing DeltaQ project"),
        tr("Open an existing DeltaQ project"));
    setActionPresentation(
        registerAction("file.save", tr("Save"), QKeySequence::Save, "file"),
        QStyle::SP_DialogSaveButton,
        ":/icons/toolbar/save.svg",
        tr("Save the current file or editor surface"),
        tr("Save the current file or editor surface"));
    setActionPresentation(
        registerAction("file.saveAll", tr("Save All"), QKeySequence("Ctrl+Shift+S"), "file"),
        QStyle::SP_DialogApplyButton,
        {},
        tr("Save all modified project files"),
        tr("Save all modified project files"));
    setActionPresentation(
        registerAction("file.close", tr("Close"), QKeySequence::Close, "file"),
        QStyle::SP_DialogCloseButton,
        {},
        tr("Close the current project"),
        tr("Close the current project"));
    setActionPresentation(
        registerAction("file.quit", tr("Quit"), QKeySequence::Quit, "file"),
        QStyle::SP_DialogCloseButton,
        {},
        tr("Quit DeltaQ"),
        tr("Quit DeltaQ"));

    // Группа "project"
    setActionPresentation(
        registerAction("project.properties", tr("Project Properties..."), QKeySequence("Alt+Return"), "project"),
        QStyle::SP_FileDialogDetailedView,
        ":/icons/toolbar/project_properties.svg",
        tr("Edit project build settings and local toolchain selection"),
        tr("Edit project build settings and local toolchain selection"));
    setActionPresentation(
        registerAction("project.rescanToolchains", tr("Rescan Toolchains"), {}, "project"),
        QStyle::SP_BrowserReload,
        {},
        tr("Rescan CMake, compilers and builders on this machine"),
        tr("Rescan CMake, compilers and builders on this machine"));
    setActionPresentation(
        registerAction("project.openBuildDirectory", tr("Open Build Directory"), {}, "project"),
        QStyle::SP_DirOpenIcon,
        {},
        tr("Open the current project's build directory"),
        tr("Open the current project's build directory"));
    setActionPresentation(
        registerAction("project.openDistDirectory", tr("Open Dist Directory"), {}, "project"),
        QStyle::SP_DirOpenIcon,
        {},
        tr("Open the current project's dist directory"),
        tr("Open the current project's dist directory"));

    // Группа "edit"
    setActionPresentation(registerAction("edit.undo", tr("Undo"), QKeySequence::Undo, "edit"),
                          QStyle::SP_ArrowBack,
                          ":/icons/toolbar/undo.svg",
                          tr("Undo the last operation"),
                          tr("Undo the last operation"));
    setActionPresentation(registerAction("edit.redo", tr("Redo"), QKeySequence::Redo, "edit"),
                          QStyle::SP_ArrowForward,
                          ":/icons/toolbar/redo.svg",
                          tr("Redo the last undone operation"),
                          tr("Redo the last undone operation"));
    setActionPresentation(registerAction("edit.cut", tr("Cut"), QKeySequence::Cut, "edit"),
                          QStyle::SP_TrashIcon,
                          {},
                          tr("Cut the current selection"),
                          tr("Cut the current selection"));
    setActionPresentation(registerAction("edit.copy", tr("Copy"), QKeySequence::Copy, "edit"),
                          QStyle::SP_FileDialogContentsView,
                          {},
                          tr("Copy the current selection"),
                          tr("Copy the current selection"));
    setActionPresentation(registerAction("edit.paste", tr("Paste"), QKeySequence::Paste, "edit"),
                          QStyle::SP_DialogOpenButton,
                          {},
                          tr("Paste from the clipboard"),
                          tr("Paste from the clipboard"));
    setActionPresentation(registerAction("edit.find", tr("Find..."), QKeySequence::Find, "edit"),
                          QStyle::SP_FileDialogContentsView,
                          {},
                          tr("Find text in the current editor"),
                          tr("Find text in the current editor"));
    setActionPresentation(registerAction("edit.replace", tr("Replace..."), QKeySequence::Replace, "edit"),
                          QStyle::SP_BrowserReload,
                          {},
                          tr("Find and replace text in the current editor"),
                          tr("Find and replace text in the current editor"));
    setActionPresentation(registerAction("edit.goToLine", tr("Go to Line..."), QKeySequence("Ctrl+G"), "edit"),
                          QStyle::SP_ArrowDown,
                          {},
                          tr("Jump to a specific line"),
                          tr("Jump to a specific line"));
    setActionPresentation(registerAction("edit.goToDefinition", tr("Go to Definition"), QKeySequence("F12"), "edit"),
                          QStyle::SP_ArrowForward,
                          {},
                          tr("Jump to the symbol definition"),
                          tr("Jump to the symbol definition"));
    setActionPresentation(registerAction("edit.findReferences", tr("Find References"), QKeySequence("Shift+F12"), "edit"),
                          QStyle::SP_FileDialogListView,
                          {},
                          tr("Find symbol references"),
                          tr("Find symbol references"));
    setActionPresentation(registerAction("edit.openGeneratedOrigin", tr("Open Generated Origin"), {}, "edit"),
                          QStyle::SP_FileLinkIcon,
                          {},
                          tr("Open the source graph or UI layout for a generated file"),
                          tr("Open the source graph or UI layout for a generated file"));
    setActionPresentation(registerAction("edit.rename", tr("Rename Symbol"), QKeySequence("F2"), "edit"),
                          QStyle::SP_FileDialogDetailedView,
                          {},
                          tr("Rename the symbol under the cursor"),
                          tr("Rename the symbol under the cursor"));
    setActionPresentation(registerAction("edit.format", tr("Format Document"), QKeySequence("Ctrl+Shift+I"), "edit"),
                          QStyle::SP_DialogResetButton,
                          {},
                          tr("Format the current document"),
                          tr("Format the current document"));

    // Группа "build"
    setActionPresentation(registerAction("build.build", tr("Build"), QKeySequence("Ctrl+B"), "build"),
                          QStyle::SP_ComputerIcon,
                          ":/icons/toolbar/build.svg",
                          tr("Run pre-build and compile the current project"),
                          tr("Run pre-build and compile the current project"));
    setActionPresentation(registerAction("build.run", tr("Run"), QKeySequence("Ctrl+R"), "build"),
                          QStyle::SP_MediaPlay,
                          ":/icons/toolbar/run.svg",
                          tr("Run the current project executable"),
                          tr("Run the current project executable"));
    setActionPresentation(registerAction("build.exportLinuxBundle", tr("Export Linux Bundle"), {}, "build"),
                          QStyle::SP_DialogSaveButton,
                          ":/icons/toolbar/export_bundle.svg",
                          tr("Build and export a runnable Linux bundle"),
                          tr("Build and export a runnable Linux bundle"));
    setActionPresentation(registerAction("build.clean", tr("Clean"), QKeySequence("Ctrl+Shift+B"), "build"),
                          QStyle::SP_DialogResetButton,
                          {},
                          tr("Remove the current build directory"),
                          tr("Remove the current build directory"));

    // Группа "debug"
    setActionPresentation(registerAction("debug.start", tr("Start Debugging"), QKeySequence("F5"), "debug"),
                          QStyle::SP_MediaPlay,
                          {},
                          tr("Build and start a debug session"),
                          tr("Build and start a debug session"));
    setActionPresentation(registerAction("debug.stop", tr("Stop Debugging"), QKeySequence("Shift+F5"), "debug"),
                          QStyle::SP_MediaStop,
                          {},
                          tr("Stop the active debug session"),
                          tr("Stop the active debug session"));
    setActionPresentation(registerAction("debug.continue", tr("Continue"), QKeySequence("F5"), "debug"),
                          QStyle::SP_MediaSeekForward,
                          {},
                          tr("Continue program execution"),
                          tr("Continue program execution"));
    setActionPresentation(registerAction("debug.stepOver", tr("Step Over"), QKeySequence("F10"), "debug"),
                          QStyle::SP_ArrowForward,
                          {},
                          tr("Step over the current line"),
                          tr("Step over the current line"));
    setActionPresentation(registerAction("debug.stepInto", tr("Step Into"), QKeySequence("F11"), "debug"),
                          QStyle::SP_ArrowDown,
                          {},
                          tr("Step into the current call"),
                          tr("Step into the current call"));
    setActionPresentation(registerAction("debug.stepOut", tr("Step Out"), QKeySequence("Shift+F11"), "debug"),
                          QStyle::SP_ArrowUp,
                          {},
                          tr("Step out of the current call"),
                          tr("Step out of the current call"));
    setActionPresentation(registerAction("debug.toggleBreakpoint", tr("Toggle Breakpoint"), QKeySequence("F9"), "debug"),
                          QStyle::SP_MessageBoxWarning,
                          {},
                          tr("Toggle a breakpoint on the current line"),
                          tr("Toggle a breakpoint on the current line"));

    // Группа "view"
    setActionPresentation(registerAction("view.codeEditor", tr("Code Editor"), QKeySequence("Ctrl+1"), "view"),
                          QStyle::SP_FileIcon,
                          ":/icons/toolbar/code_editor.svg",
                          tr("Switch to the code editor"),
                          tr("Switch to the code editor"));
    setActionPresentation(registerAction("view.blockEditor", tr("Block Editor"), QKeySequence("Ctrl+2"), "view"),
                          QStyle::SP_DirIcon,
                          ":/icons/toolbar/block_editor.svg",
                          tr("Switch to the block editor"),
                          tr("Switch to the block editor"));
    setActionPresentation(registerAction("view.uiDesigner", tr("UI Designer"), QKeySequence("Ctrl+3"), "view"),
                          QStyle::SP_TitleBarMenuButton,
                          ":/icons/toolbar/ui_designer.svg",
                          tr("Switch to the UI designer"),
                          tr("Switch to the UI designer"));
    setActionPresentation(registerAction("view.libProcessor", tr("Library Processor"), QKeySequence("Ctrl+4"), "view"),
                          QStyle::SP_DriveHDIcon,
                          ":/icons/toolbar/library_processor.svg",
                          tr("Switch to the library processor"),
                          tr("Switch to the library processor"));
}

} // namespace DeltaQ

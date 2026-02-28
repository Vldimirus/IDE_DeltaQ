#pragma once

#include <QObject>
#include <QMap>
#include <QAction>
#include <QString>
#include <QKeySequence>

namespace DeltaQ {

class ActionManager : public QObject {
    Q_OBJECT

public:
    explicit ActionManager(QObject *parent = nullptr);

    QAction *registerAction(const QString &id, const QString &text,
                            const QKeySequence &shortcut = {});
    QAction *action(const QString &id) const;
    QList<QAction *> allActions() const;

    // Standard actions
    void setupStandardActions();
    QAction *newProjectAction() const { return action("file.newProject"); }
    QAction *openProjectAction() const { return action("file.openProject"); }
    QAction *saveAction() const { return action("file.save"); }
    QAction *saveAllAction() const { return action("file.saveAll"); }
    QAction *undoAction() const { return action("edit.undo"); }
    QAction *redoAction() const { return action("edit.redo"); }
    QAction *buildAction() const { return action("build.build"); }
    QAction *runAction() const { return action("build.run"); }

private:
    QMap<QString, QAction *> m_actions;
};

} // namespace DeltaQ

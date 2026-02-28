#pragma once

#include <QObject>
#include <QMap>
#include <QSet>
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

    // Регистрация действия с привязкой к группе
    QAction *registerAction(const QString &id, const QString &text,
                            const QKeySequence &shortcut,
                            const QString &group);

    QAction *action(const QString &id) const;
    QList<QAction *> allActions() const;

    // Группы действий
    void addToGroup(const QString &actionId, const QString &groupId);
    void enableGroup(const QString &groupId);
    void disableGroup(const QString &groupId);
    QList<QAction *> actionsInGroup(const QString &groupId) const;

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
    QMap<QString, QSet<QString>> m_groups;
};

} // namespace DeltaQ

#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <deltaq/UILayout.h>

namespace DeltaQ {

// Хранилище UI-макетов — управление коллекцией UILayout с файловой персистентностью (.dqui)
class UILayoutStore : public QObject {
    Q_OBJECT

public:
    explicit UILayoutStore(QObject *parent = nullptr);

    // Управление макетами
    bool registerLayout(const UILayout &layout);
    bool unregisterLayout(const QString &id);
    UILayout *findLayout(const QString &id);
    const UILayout *findLayout(const QString &id) const;
    UILayout *findLayoutByName(const QString &name);
    QVector<const UILayout *> allLayouts() const;

    // Файловый I/O
    bool loadLayoutFile(const QString &dquiPath);
    bool saveLayoutFile(const UILayout &layout, const QString &dquiPath) const;

    // Загрузка всех .dqui из директории проекта
    bool loadFromDirectory(const QString &projectDir);
    bool saveAll(const QString &projectDir) const;

    int count() const { return m_layouts.size(); }
    void clear();

signals:
    void layoutRegistered(const QString &id);
    void layoutUnregistered(const QString &id);
    void layoutUpdated(const QString &id);
    void storeCleared();

private:
    QMap<QString, UILayout> m_layouts; // id -> UILayout
};

} // namespace DeltaQ

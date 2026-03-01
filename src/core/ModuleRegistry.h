#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <deltaq/Module.h>

namespace DeltaQ {

// Метаданные пакета модулей
struct ModulePack {
    QString name;
    QString version;
    QString author;
    QString description;
    QString path;       // путь к папке пакета
    bool isCore = false;
};

class ModuleRegistry : public QObject {
    Q_OBJECT

public:
    explicit ModuleRegistry(QObject *parent = nullptr);

    // Module management
    bool registerModule(const Module &module);
    bool unregisterModule(const QString &id);
    Module *findModule(const QString &id);
    const Module *findModule(const QString &id) const;
    Module *findModuleByName(const QString &name);
    Module *findBySourcePath(const QString &sourcePath);
    QVector<const Module *> allModules() const;
    QVector<const Module *> modulesByOrigin(const QString &origin) const;
    QVector<const Module *> modulesByCategory(const QString &category) const;
    QStringList categories() const;

    // I/O
    bool loadFromFile(const QString &path);
    bool saveToFile(const QString &path) const;
    bool loadModuleFile(const QString &dqmodPath);
    bool saveModuleFile(const Module &module, const QString &dqmodPath) const;

    // Загрузка глобальных модулей (core + расширения)
    void loadGlobalModules(const QString &globalModulesDir);

    // Загрузка локальных модулей проекта (из dqmods/)
    void loadLocalModules(const QString &dqmodsDir);

    // Очистка локальных модулей (при закрытии проекта)
    void clearLocalModules();

    // Registry file (.dqreg) — проектные данные (аннотации)
    bool loadRegistry(const QString &projectDir);
    bool saveRegistry(const QString &projectDir) const;

    // Список установленных пакетов
    QVector<ModulePack> installedPacks() const;

    // Проверка типа модуля
    bool isCoreModule(const QString &id) const;
    bool isExtensionModule(const QString &id) const;

    // Валидация
    bool validateModule(const Module &module) const;
    bool hasDuplicateName(const QString &name, const QString &excludeId = QString()) const;

    int count() const { return m_modules.size(); }
    void clear();

signals:
    void moduleRegistered(const QString &id);
    void moduleUnregistered(const QString &id);
    void moduleUpdated(const QString &id);
    void registryCleared();

private:
    QMap<QString, Module> m_modules;        // id -> Module
    QVector<ModulePack> m_packs;            // установленные пакеты
};

} // namespace DeltaQ

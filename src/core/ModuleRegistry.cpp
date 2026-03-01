#include "ModuleRegistry.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

namespace DeltaQ {

ModuleRegistry::ModuleRegistry(QObject *parent)
    : QObject(parent)
{
}

bool ModuleRegistry::registerModule(const Module &module)
{
    if (module.id.isEmpty())
        return false;

    bool isUpdate = m_modules.contains(module.id);
    m_modules[module.id] = module;

    if (isUpdate)
        emit moduleUpdated(module.id);
    else
        emit moduleRegistered(module.id);
    return true;
}

bool ModuleRegistry::unregisterModule(const QString &id)
{
    if (m_modules.remove(id) > 0) {
        emit moduleUnregistered(id);
        return true;
    }
    return false;
}

Module *ModuleRegistry::findModule(const QString &id)
{
    auto it = m_modules.find(id);
    return it != m_modules.end() ? &it.value() : nullptr;
}

const Module *ModuleRegistry::findModule(const QString &id) const
{
    auto it = m_modules.find(id);
    return it != m_modules.end() ? &it.value() : nullptr;
}

Module *ModuleRegistry::findModuleByName(const QString &name)
{
    for (auto &m : m_modules) {
        if (m.name == name)
            return &m;
    }
    return nullptr;
}

QVector<const Module *> ModuleRegistry::allModules() const
{
    QVector<const Module *> result;
    result.reserve(m_modules.size());
    for (const auto &m : m_modules)
        result.append(&m);
    return result;
}

Module *ModuleRegistry::findBySourcePath(const QString &sourcePath)
{
    for (auto &m : m_modules) {
        if (m.sourcePath == sourcePath)
            return &m;
    }
    return nullptr;
}

QVector<const Module *> ModuleRegistry::modulesByOrigin(const QString &origin) const
{
    QVector<const Module *> result;
    for (const auto &m : m_modules) {
        if (m.origin == origin)
            result.append(&m);
    }
    return result;
}

QVector<const Module *> ModuleRegistry::modulesByCategory(const QString &category) const
{
    QVector<const Module *> result;
    for (const auto &m : m_modules) {
        if (m.category == category)
            result.append(&m);
    }
    return result;
}

QStringList ModuleRegistry::categories() const
{
    QSet<QString> cats;
    for (const auto &m : m_modules) {
        if (!m.category.isEmpty())
            cats.insert(m.category);
    }
    return QStringList(cats.begin(), cats.end());
}

bool ModuleRegistry::validateModule(const Module &module) const
{
    return module.isValid();
}

bool ModuleRegistry::hasDuplicateName(const QString &name, const QString &excludeId) const
{
    for (const auto &m : m_modules) {
        if (m.name == name && m.id != excludeId)
            return true;
    }
    return false;
}

bool ModuleRegistry::loadModuleFile(const QString &dqmodPath)
{
    QFile file(dqmodPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return false;

    Module mod = Module::fromJson(doc.object());
    return registerModule(mod);
}

bool ModuleRegistry::saveModuleFile(const Module &module, const QString &dqmodPath) const
{
    QFile file(dqmodPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(module.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

void ModuleRegistry::loadGlobalModules(const QString &globalModulesDir)
{
    QDir rootDir(globalModulesDir);
    if (!rootDir.exists()) return;

    m_packs.clear();

    // Сканируем подпапки первого уровня (core/, my_utils/, sensors_pack/, ...)
    QStringList packDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &packName : packDirs) {
        QString packPath = globalModulesDir + "/" + packName;
        bool isCore = (packName == "core");

        // Читаем pack.json
        ModulePack pack;
        pack.path = packPath;
        pack.isCore = isCore;
        pack.name = packName;

        QFile packFile(packPath + "/pack.json");
        if (packFile.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            auto doc = QJsonDocument::fromJson(packFile.readAll(), &err);
            if (err.error == QJsonParseError::NoError) {
                auto obj = doc.object();
                pack.name = obj["name"].toString(packName);
                pack.version = obj["version"].toString();
                pack.author = obj["author"].toString();
                pack.description = obj["description"].toString();
            }
        }

        m_packs.append(pack);

        // Загружаем все .dqmod рекурсивно из папки пакета
        QDirIterator it(packPath, {"*.dqmod"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) continue;

            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(file.readAll(), &error);
            if (error.error != QJsonParseError::NoError) continue;

            Module mod = Module::fromJson(doc.object());
            // Устанавливаем origin в зависимости от типа пакета
            if (isCore)
                mod.origin = "core";
            else
                mod.origin = "extension";

            if (!mod.id.isEmpty())
                registerModule(mod);
        }
    }
}

QVector<ModulePack> ModuleRegistry::installedPacks() const
{
    return m_packs;
}

bool ModuleRegistry::isCoreModule(const QString &id) const
{
    auto it = m_modules.find(id);
    if (it == m_modules.end()) return false;
    return it->origin == "core";
}

bool ModuleRegistry::isExtensionModule(const QString &id) const
{
    auto it = m_modules.find(id);
    if (it == m_modules.end()) return false;
    return it->origin == "extension";
}

bool ModuleRegistry::loadRegistry(const QString &projectDir)
{
    // Загружаем .dqmod из проекта (пользовательские модули из аннотаций)
    QDirIterator it(projectDir + "/src", {"*.dqmod"}, QDir::Files, QDirIterator::Subdirectories);
    bool anyLoaded = false;
    while (it.hasNext()) {
        if (loadModuleFile(it.next()))
            anyLoaded = true;
    }
    return anyLoaded;
}

bool ModuleRegistry::saveRegistry(const QString &projectDir) const
{
    // Save registry index file
    QString regPath = projectDir + "/modules.dqreg";
    QFile file(regPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonArray arr;
    for (const auto &m : m_modules)
        arr.append(m.toJson());

    QJsonObject root;
    root["version"] = "1.0";
    root["modules"] = arr;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool ModuleRegistry::loadFromFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return false;

    auto root = doc.object();
    for (const auto &v : root["modules"].toArray()) {
        Module mod = Module::fromJson(v.toObject());
        m_modules[mod.id] = mod;
    }
    return true;
}

bool ModuleRegistry::saveToFile(const QString &path) const
{
    return saveRegistry(QFileInfo(path).absolutePath());
}

void ModuleRegistry::loadLocalModules(const QString &dqmodsDir)
{
    QDir dir(dqmodsDir);
    if (!dir.exists()) return;

    QDirIterator it(dqmodsDir, {"*.dqmod"}, QDir::Files);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) continue;

        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) continue;

        Module mod = Module::fromJson(doc.object());
        mod.origin = "local";
        if (!mod.id.isEmpty())
            registerModule(mod);
    }
}

void ModuleRegistry::clearLocalModules()
{
    QStringList toRemove;
    for (auto it = m_modules.begin(); it != m_modules.end(); ++it) {
        if (it->origin == "local")
            toRemove.append(it.key());
    }
    for (const auto &id : toRemove) {
        m_modules.remove(id);
        emit moduleUnregistered(id);
    }
}

void ModuleRegistry::clear()
{
    m_modules.clear();
    emit registryCleared();
}

} // namespace DeltaQ

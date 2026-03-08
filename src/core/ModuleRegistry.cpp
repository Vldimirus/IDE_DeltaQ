#include "ModuleRegistry.h"
#include "GraphStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

namespace DeltaQ {

namespace {

// Проверяет, что boundary metadata составного модуля согласована с его контрактом и внутренним графом.
bool validateCompositeBoundary(const ModuleRegistry *registry,
                               const Graph &innerGraph,
                               const Module &module,
                               QString *reason)
{
    const auto validateBindings = [&](const QVector<Port> &externalPorts,
                                      const QVector<CompositePortBinding> &bindings,
                                      bool isInputSide) {
        if (externalPorts.size() != bindings.size()) {
            if (reason) {
                *reason = QObject::tr("boundary mapping count does not match %1 ports")
                              .arg(isInputSide ? QObject::tr("input") : QObject::tr("output"));
            }
            return false;
        }

        QSet<QString> seenExternalPorts;
        for (const auto &binding : bindings) {
            if (binding.externalPortName.isEmpty() ||
                binding.internalNodeId.isEmpty() ||
                binding.internalPortName.isEmpty()) {
                if (reason)
                    *reason = QObject::tr("boundary mapping contains empty fields");
                return false;
            }

            if (seenExternalPorts.contains(binding.externalPortName)) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping duplicates external port '%1'")
                                  .arg(binding.externalPortName);
                }
                return false;
            }
            seenExternalPorts.insert(binding.externalPortName);

            const Port *externalPort = isInputSide
                ? module.findInput(binding.externalPortName)
                : module.findOutput(binding.externalPortName);
            if (!externalPort) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping references unknown external port '%1'")
                                  .arg(binding.externalPortName);
                }
                return false;
            }

            const GraphNode *innerNode = innerGraph.findNode(binding.internalNodeId);
            if (!innerNode) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping references missing inner node '%1'")
                                  .arg(binding.internalNodeId);
                }
                return false;
            }

            const Module *innerModule = registry->findModule(innerNode->moduleId);
            if (!innerModule) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping references missing inner module '%1'")
                                  .arg(innerNode->moduleId);
                }
                return false;
            }

            const Port *internalPort = isInputSide
                ? innerModule->findInput(binding.internalPortName)
                : innerModule->findOutput(binding.internalPortName);
            if (!internalPort) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping references invalid inner port '%1'")
                                  .arg(binding.internalPortName);
                }
                return false;
            }

            if (externalPort->type != internalPort->type) {
                if (reason) {
                    *reason = QObject::tr("boundary type mismatch for port '%1'")
                                  .arg(binding.externalPortName);
                }
                return false;
            }

            if (externalPort->kind != internalPort->kind || externalPort->kind != binding.kind) {
                if (reason) {
                    *reason = QObject::tr("boundary kind mismatch for port '%1'")
                                  .arg(binding.externalPortName);
                }
                return false;
            }
        }

        for (const auto &externalPort : externalPorts) {
            if (!seenExternalPorts.contains(externalPort.name)) {
                if (reason) {
                    *reason = QObject::tr("boundary mapping is missing for external port '%1'")
                                  .arg(externalPort.name);
                }
                return false;
            }
        }

        return true;
    };

    if (!validateBindings(module.inputs, module.boundaryInputs, true))
        return false;
    if (!validateBindings(module.outputs, module.boundaryOutputs, false))
        return false;

    if (reason)
        reason->clear();
    return true;
}

// Рекурсивно проверяет, что модуль действительно готов к композиции.
// Для составных модулей дополнительно проверяется внутренний граф и все вложенные модули.
bool isModuleAdmittedRecursive(const ModuleRegistry *registry,
                               const GraphStore *graphStore,
                               const Module &module,
                               QString *reason,
                               QSet<QString> &activeCompositeModules)
{
    // Core/UI считаются доверенными библиотечными модулями и допускаются по умолчанию.
    if (module.origin == "core" || module.origin == "ui") {
        if (reason)
            reason->clear();
        return true;
    }

    if (!registry->validateModule(module)) {
        if (reason)
            *reason = QObject::tr("module contract is invalid");
        return false;
    }

    if (!registry->moduleHasImplementation(module)) {
        if (reason)
            *reason = QObject::tr("module implementation is missing");
        return false;
    }

    // Составной модуль должен иметь реальный внутренний граф и все его узлы
    // должны ссылаться только на уже допущенные модули.
    if (module.isComposite()) {
        if (!graphStore) {
            if (reason)
                *reason = QObject::tr("graph store is not attached");
            return false;
        }

        if (activeCompositeModules.contains(module.id)) {
            if (reason)
                *reason = QObject::tr("composite module dependency cycle detected");
            return false;
        }

        const Graph *innerGraph = graphStore->findGraph(module.graphId);
        if (!innerGraph) {
            if (reason)
                *reason = QObject::tr("inner graph '%1' not found").arg(module.graphId);
            return false;
        }

        if (!innerGraph->isValid()) {
            if (reason)
                *reason = QObject::tr("inner graph is invalid");
            return false;
        }

        if (!innerGraph->parentModuleId.isEmpty() && innerGraph->parentModuleId != module.id) {
            if (reason) {
                *reason = QObject::tr("inner graph belongs to another module ('%1')")
                              .arg(innerGraph->parentModuleId);
            }
            return false;
        }

        if (!validateCompositeBoundary(registry, *innerGraph, module, reason))
            return false;

        activeCompositeModules.insert(module.id);

        for (const auto &node : innerGraph->nodes) {
            const Module *innerModule = registry->findModule(node.moduleId);
            if (!innerModule) {
                activeCompositeModules.remove(module.id);
                if (reason) {
                    *reason = QObject::tr("inner node '%1' references missing module '%2'")
                                  .arg(node.id, node.moduleId);
                }
                return false;
            }

            QString innerReason;
            if (!isModuleAdmittedRecursive(registry, graphStore, *innerModule,
                                           &innerReason, activeCompositeModules)) {
                activeCompositeModules.remove(module.id);
                if (reason) {
                    *reason = QObject::tr("inner node '%1' uses module '%2' that is not admitted: %3")
                                  .arg(node.id, innerModule->name, innerReason);
                }
                return false;
            }
        }

        for (const auto &conn : innerGraph->connections) {
            const GraphNode *fromNode = innerGraph->findNode(conn.from.nodeId);
            const GraphNode *toNode = innerGraph->findNode(conn.to.nodeId);
            if (!fromNode || !toNode) {
                activeCompositeModules.remove(module.id);
                if (reason)
                    *reason = QObject::tr("inner graph contains dangling connections");
                return false;
            }

            const Module *fromModule = registry->findModule(fromNode->moduleId);
            const Module *toModule = registry->findModule(toNode->moduleId);
            if (!fromModule || !toModule ||
                !fromModule->hasOutput(conn.from.portName) ||
                !toModule->hasInput(conn.to.portName)) {
                activeCompositeModules.remove(module.id);
                if (reason)
                    *reason = QObject::tr("inner graph contains invalid port connections");
                return false;
            }
        }

        activeCompositeModules.remove(module.id);
        if (reason)
            reason->clear();
        return true;
    }

    // Пользовательский атомарный модуль допускается в сборку только после
    // независимого прохождения compile check и test check.
    if (module.compileStatus != "passed") {
        if (reason) {
            *reason = QObject::tr("module is not admitted to composition until compile check passes "
                                  "(current status: %1)").arg(module.compileStatus);
        }
        return false;
    }

    if (module.testStatus != "passed") {
        if (reason) {
            *reason = QObject::tr("module is not admitted to composition until test check passes "
                                  "(current status: %1)").arg(module.testStatus);
        }
        return false;
    }

    if (reason)
        reason->clear();
    return true;
}

} // namespace

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

bool ModuleRegistry::moduleHasImplementation(const Module &module) const
{
    // Реализация может быть текстовой, графовой или ссылкой на внешний исходник.
    return !module.sourceCode.trimmed().isEmpty()
        || !module.graphId.trimmed().isEmpty()
        || !module.sourcePath.trimmed().isEmpty();
}

bool ModuleRegistry::isModuleAdmittedForComposition(const Module &module, QString *reason) const
{
    QSet<QString> activeCompositeModules;
    return isModuleAdmittedRecursive(this, m_graphStore, module, reason, activeCompositeModules);
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
    mod.storagePath = dqmodPath;
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
            mod.storagePath = filePath;

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
        mod.storagePath = filePath;
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

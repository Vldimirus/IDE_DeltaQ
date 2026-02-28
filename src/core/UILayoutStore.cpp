#include "UILayoutStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QDirIterator>

namespace DeltaQ {

UILayoutStore::UILayoutStore(QObject *parent)
    : QObject(parent)
{
}

bool UILayoutStore::registerLayout(const UILayout &layout)
{
    if (layout.id.isEmpty())
        return false;

    bool isUpdate = m_layouts.contains(layout.id);
    m_layouts[layout.id] = layout;

    if (isUpdate)
        emit layoutUpdated(layout.id);
    else
        emit layoutRegistered(layout.id);
    return true;
}

bool UILayoutStore::unregisterLayout(const QString &id)
{
    if (m_layouts.remove(id) > 0) {
        emit layoutUnregistered(id);
        return true;
    }
    return false;
}

UILayout *UILayoutStore::findLayout(const QString &id)
{
    auto it = m_layouts.find(id);
    return it != m_layouts.end() ? &it.value() : nullptr;
}

const UILayout *UILayoutStore::findLayout(const QString &id) const
{
    auto it = m_layouts.find(id);
    return it != m_layouts.end() ? &it.value() : nullptr;
}

UILayout *UILayoutStore::findLayoutByName(const QString &name)
{
    for (auto &l : m_layouts) {
        if (l.name == name)
            return &l;
    }
    return nullptr;
}

QVector<const UILayout *> UILayoutStore::allLayouts() const
{
    QVector<const UILayout *> result;
    result.reserve(m_layouts.size());
    for (const auto &l : m_layouts)
        result.append(&l);
    return result;
}

bool UILayoutStore::loadLayoutFile(const QString &dquiPath)
{
    QFile file(dquiPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
        return false;

    UILayout layout = UILayout::fromJson(doc.object());
    return registerLayout(layout);
}

bool UILayoutStore::saveLayoutFile(const UILayout &layout, const QString &dquiPath) const
{
    QFile file(dquiPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QJsonDocument doc(layout.toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool UILayoutStore::loadFromDirectory(const QString &projectDir)
{
    QDirIterator it(projectDir, {"*.dqui"}, QDir::Files, QDirIterator::Subdirectories);
    bool anyLoaded = false;
    while (it.hasNext()) {
        if (loadLayoutFile(it.next()))
            anyLoaded = true;
    }
    return anyLoaded;
}

bool UILayoutStore::saveAll(const QString &projectDir) const
{
    QDir dir(projectDir + "/ui");
    if (!dir.exists())
        dir.mkpath(".");

    for (const auto &l : m_layouts) {
        QString path = dir.absoluteFilePath(l.name + ".dqui");
        if (!saveLayoutFile(l, path))
            return false;
    }
    return true;
}

void UILayoutStore::clear()
{
    m_layouts.clear();
    emit storeCleared();
}

} // namespace DeltaQ

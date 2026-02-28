#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

namespace DeltaQ {

struct BuildConfig {
    QString compiler = "gcc";
    QString standard = "c17";
    QString outputDir = "build/";
    QStringList flags;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["compiler"] = compiler;
        obj["standard"] = standard;
        obj["output"] = outputDir;
        QJsonArray f;
        for (const auto &fl : flags)
            f.append(fl);
        obj["flags"] = f;
        return obj;
    }

    static BuildConfig fromJson(const QJsonObject &obj) {
        BuildConfig bc;
        bc.compiler = obj["compiler"].toString("gcc");
        bc.standard = obj["standard"].toString("c17");
        bc.outputDir = obj["output"].toString("build/");
        for (const auto &v : obj["flags"].toArray())
            bc.flags.append(v.toString());
        return bc;
    }

    bool operator==(const BuildConfig &other) const {
        return compiler == other.compiler
            && standard == other.standard
            && outputDir == other.outputDir
            && flags == other.flags;
    }
};

struct Project {
    QString name;
    QString version;
    QString projectFilePath;
    QString projectDir;
    QString projectType;          // "console", "desktop"
    QStringList moduleGlobs;
    QStringList graphGlobs;
    QStringList uiLayoutGlobs;
    BuildConfig build;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["version"] = version;

        auto toArr = [](const QStringList &list) {
            QJsonArray arr;
            for (const auto &s : list)
                arr.append(s);
            return arr;
        };

        obj["modules"] = toArr(moduleGlobs);
        obj["graphs"] = toArr(graphGlobs);
        obj["ui_layouts"] = toArr(uiLayoutGlobs);
        if (!projectType.isEmpty())
            obj["type"] = projectType;
        obj["build"] = build.toJson();
        return obj;
    }

    static Project fromJson(const QJsonObject &obj) {
        Project p;
        p.name = obj["name"].toString();
        p.version = obj["version"].toString("1.0.0");

        auto toList = [](const QJsonArray &arr) {
            QStringList list;
            for (const auto &v : arr)
                list.append(v.toString());
            return list;
        };

        p.moduleGlobs = toList(obj["modules"].toArray());
        p.graphGlobs = toList(obj["graphs"].toArray());
        p.uiLayoutGlobs = toList(obj["ui_layouts"].toArray());
        p.projectType = obj["type"].toString("console");
        p.build = BuildConfig::fromJson(obj["build"].toObject());
        return p;
    }

    bool operator==(const Project &other) const {
        return name == other.name && version == other.version;
    }

    // Валидация: name не пуст
    bool isValid() const {
        return !name.isEmpty();
    }

    static Project createNew(const QString &name) {
        Project p;
        p.name = name;
        p.version = "1.0.0";
        p.moduleGlobs = {"modules/*.dqmod"};
        p.graphGlobs = {"graphs/*.dqgraph"};
        p.uiLayoutGlobs = {"ui/*.dqui"};
        return p;
    }
};

} // namespace DeltaQ

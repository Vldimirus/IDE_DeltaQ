#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QRectF>
#include <QUuid>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QtMath>

#include <deltaq/UIContract.h>

namespace DeltaQ {

struct UIAnchors {
    bool left = false, right = false, top = false, bottom = false;
    bool hCenter = false, vCenter = false;
    qreal leftMargin = 0, rightMargin = 0, topMargin = 0, bottomMargin = 0;

    bool hasAnchors() const {
        return left || right || top || bottom || hCenter || vCenter;
    }

    bool operator==(const UIAnchors &other) const {
        return left == other.left && right == other.right
            && top == other.top && bottom == other.bottom
            && hCenter == other.hCenter && vCenter == other.vCenter
            && qFuzzyCompare(leftMargin, other.leftMargin)
            && qFuzzyCompare(rightMargin, other.rightMargin)
            && qFuzzyCompare(topMargin, other.topMargin)
            && qFuzzyCompare(bottomMargin, other.bottomMargin);
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        if (left) obj["left"] = true;
        if (right) obj["right"] = true;
        if (top) obj["top"] = true;
        if (bottom) obj["bottom"] = true;
        if (hCenter) obj["hCenter"] = true;
        if (vCenter) obj["vCenter"] = true;
        if (leftMargin != 0) obj["leftMargin"] = leftMargin;
        if (rightMargin != 0) obj["rightMargin"] = rightMargin;
        if (topMargin != 0) obj["topMargin"] = topMargin;
        if (bottomMargin != 0) obj["bottomMargin"] = bottomMargin;
        return obj;
    }

    static UIAnchors fromJson(const QJsonObject &obj) {
        UIAnchors a;
        a.left = obj["left"].toBool();
        a.right = obj["right"].toBool();
        a.top = obj["top"].toBool();
        a.bottom = obj["bottom"].toBool();
        a.hCenter = obj["hCenter"].toBool();
        a.vCenter = obj["vCenter"].toBool();
        a.leftMargin = obj["leftMargin"].toDouble();
        a.rightMargin = obj["rightMargin"].toDouble();
        a.topMargin = obj["topMargin"].toDouble();
        a.bottomMargin = obj["bottomMargin"].toDouble();
        return a;
    }
};

struct UIWidget {
    QString id;
    QString type;         // "Button", "TextField", "Label", "Panel", ...
    QString name;
    QRectF geometry;      // x, y, width, height
    QString layout;       // "None", "HBox", "VBox", "Grid", "Flow"
    QMap<QString, QVariant> properties;
    QMap<QString, QString> events;   // событие → обработчик
    QJsonObject metadata;            // backend-независимые contract metadata виджета
    UIAnchors anchors;               // anchor-привязки
    QVector<UIWidget> children;      // вложенные виджеты

    bool operator==(const UIWidget &other) const {
        return id == other.id
            && type == other.type
            && name == other.name
            && geometry == other.geometry
            && layout == other.layout
            && properties == other.properties
            && events == other.events
            && metadata == other.metadata
            && anchors == other.anchors
            && children == other.children;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["type"] = type;
        obj["name"] = name;
        obj["x"] = geometry.x();
        obj["y"] = geometry.y();
        obj["width"] = geometry.width();
        obj["height"] = geometry.height();
        if (!layout.isEmpty() && layout != "None")
            obj["layout"] = layout;

        if (!properties.isEmpty()) {
            QJsonObject props;
            for (auto it = properties.begin(); it != properties.end(); ++it)
                props[it.key()] = QJsonValue::fromVariant(it.value());
            obj["properties"] = props;
        }

        if (!events.isEmpty()) {
            QJsonObject ev;
            for (auto it = events.begin(); it != events.end(); ++it)
                ev[it.key()] = it.value();
            obj["events"] = ev;
        }

        if (!metadata.isEmpty())
            obj["metadata"] = metadata;

        if (anchors.hasAnchors())
            obj["anchors"] = anchors.toJson();

        if (!children.isEmpty()) {
            QJsonArray arr;
            for (const auto &child : children)
                arr.append(child.toJson());
            obj["children"] = arr;
        }

        return obj;
    }

    static UIWidget fromJson(const QJsonObject &obj) {
        UIWidget w;
        w.id = obj["id"].toString();
        w.type = obj["type"].toString();
        w.name = obj["name"].toString();
        w.geometry = QRectF(
            obj["x"].toDouble(),
            obj["y"].toDouble(),
            obj["width"].toDouble(),
            obj["height"].toDouble()
        );
        w.layout = obj["layout"].toString("None");

        auto props = obj["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it)
            w.properties[it.key()] = it.value().toVariant();

        auto ev = obj["events"].toObject();
        for (auto it = ev.begin(); it != ev.end(); ++it)
            w.events[it.key()] = it.value().toString();

        w.metadata = obj["metadata"].toObject();
        w.syncContractMetadata();

        if (obj.contains("anchors"))
            w.anchors = UIAnchors::fromJson(obj["anchors"].toObject());

        auto arr = obj["children"].toArray();
        for (const auto &v : arr)
            w.children.append(UIWidget::fromJson(v.toObject()));

        return w;
    }

    // Возвращает канонический contract type, даже если layout был сохранён в legacy-форме.
    QString contractType() const {
        const QString explicitType = metadata["deltaq.ui.contract_type"].toString();
        if (!explicitType.isEmpty())
            return explicitType;
        return canonicalUIContractType(type);
    }

    // Возвращает legacy-тип, который показывается в дизайнере и хранится в поле type.
    QString legacyWidgetType() const {
        const QString explicitType = metadata["deltaq.ui.legacy_widget_type"].toString();
        if (!explicitType.isEmpty())
            return explicitType;
        return legacyUIWidgetType(type);
    }

    // Синхронизирует metadata и field type по единому каталогу UI-контрактов.
    void syncContractMetadata() {
        const auto *spec = findUIContractSpecByAny(
            metadata["deltaq.ui.contract_type"].toString().isEmpty()
                ? type
                : metadata["deltaq.ui.contract_type"].toString());
        if (!spec)
            spec = findUIContractSpecByAny(type);
        if (!spec)
            return;

        type = spec->legacyWidgetType;
        metadata["deltaq.kind"] = "ui_widget_contract";
        metadata["deltaq.ui.layer"] = "contract";
        metadata["deltaq.ui.backend"] = "agnostic";
        metadata["deltaq.ui.contract_version"] = "1.0";
        metadata["deltaq.ui.contract_type"] = spec->contractType;
        metadata["deltaq.ui.legacy_widget_type"] = spec->legacyWidgetType;
        metadata["deltaq.ui.display_name"] = spec->displayName;
        metadata["deltaq.ui.palette_category"] = spec->paletteCategory;
    }

    // Создание виджета с id
    static UIWidget create(const QString &type, const QString &name) {
        UIWidget w;
        w.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        w.type = legacyUIWidgetType(type);
        w.name = name;
        w.layout = "None";
        w.syncContractMetadata();
        return w;
    }
};

inline constexpr int DQ_UIWindowDefaultMinWidth = 240;
inline constexpr int DQ_UIWindowDefaultMinHeight = 180;
inline constexpr bool DQ_UIWindowDefaultResizable = true;

inline QString uiWindowTitle(const UIWidget &window)
{
    const QString title = window.properties.value("title").toString().trimmed();
    if (!title.isEmpty())
        return title;
    if (!window.name.trimmed().isEmpty())
        return window.name;
    return QStringLiteral("Window");
}

inline int uiWindowMinimumWidth(const UIWidget &window)
{
    return qMax(DQ_UIWindowDefaultMinWidth,
                window.properties.value("min_width", DQ_UIWindowDefaultMinWidth).toInt());
}

inline int uiWindowMinimumHeight(const UIWidget &window)
{
    return qMax(DQ_UIWindowDefaultMinHeight,
                window.properties.value("min_height", DQ_UIWindowDefaultMinHeight).toInt());
}

inline bool uiWindowResizable(const UIWidget &window)
{
    return window.properties.value("resizable", DQ_UIWindowDefaultResizable).toBool();
}

struct UILayout {
    QString id;
    QString name;
    QString version;
    UIWidget window;        // корневой виджет (Window)
    QJsonObject resources;
    QJsonObject metadata;

    bool operator==(const UILayout &other) const {
        return id == other.id
            && name == other.name
            && version == other.version
            && window == other.window;
    }

    // Валидация: id и name не пусты
    bool isValid() const {
        return !id.isEmpty() && !name.isEmpty();
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["version"] = version;
        obj["window"] = window.toJson();
        if (!resources.isEmpty())
            obj["resources"] = resources;
        if (!metadata.isEmpty())
            obj["metadata"] = metadata;
        return obj;
    }

    static UILayout fromJson(const QJsonObject &obj) {
        UILayout l;
        l.id = obj["id"].toString();
        l.name = obj["name"].toString();
        l.version = obj["version"].toString("1.0.0");
        l.window = UIWidget::fromJson(obj["window"].toObject());
        l.resources = obj["resources"].toObject();
        l.metadata = obj["metadata"].toObject();
        return l;
    }

    // Создание макета с корневым Window
    static UILayout create(const QString &name) {
        UILayout l;
        l.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        l.name = name;
        l.version = "1.0.0";
        l.window = UIWidget::create("Window", name);
        l.window.geometry = QRectF(0, 0, 800, 600);
        // UI layout изначально описывает backend-независимый контракт,
        // а конкретный renderer/backend определяется уже на этапе codegen/runtime.
        l.metadata["deltaq.kind"] = "ui_layout_contract";
        l.metadata["deltaq.ui.contract_version"] = "1.0";
        l.metadata["deltaq.ui.layer"] = "contract";
        l.metadata["deltaq.ui.backend"] = "agnostic";
        return l;
    }
};

} // namespace DeltaQ

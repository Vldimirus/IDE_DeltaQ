#pragma once

#include <QSizeF>
#include <QString>
#include <QVariant>
#include <QVector>

namespace DeltaQ {

// Описание дизайнерского свойства UI-контракта.
struct UIContractPropertySpec {
    QString name;
    QVariant defaultValue;
    bool useWidgetNameAsDefault = false;
};

// Описание порта UI-контрактного модуля.
struct UIContractPortSpec {
    QString name;
    QString type;
    QString defaultValue;
};

// Единый словарь UI-контрактов для .dqui, палитры и UI-модулей.
struct UIContractSpec {
    QString contractType;
    QString legacyWidgetType;
    QString displayName;
    QString paletteCategory;
    QString moduleId;
    QString moduleName;
    QString description;
    QSizeF defaultSize = QSizeF(120.0, 40.0);
    QString defaultEvent;
    QVector<UIContractPropertySpec> designerProperties;
    QVector<UIContractPortSpec> moduleInputs;
    QVector<UIContractPortSpec> moduleOutputs;
    bool isContainer = false;
    bool showInPalette = true;
    bool exposeAsModule = false;
};

// Возвращает единый каталог доступных UI-контрактов.
inline const QVector<UIContractSpec> &uiContractCatalog()
{
    static const QVector<UIContractSpec> catalog = {
        {
            "window", "Window", "Window", "Root",
            {}, {}, "Корневой backend-независимый UI-контракт окна",
            QSizeF(800.0, 600.0), {},
            {
                {"title", QStringLiteral("Window"), false}
            },
            {}, {},
            true, false, false
        },
        {
            "panel", "Panel", "Panel", "Containers",
            {}, {}, "Контейнерная панель DeltaQ",
            QSizeF(200.0, 150.0), {},
            {}, {}, {},
            true, true, false
        },
        {
            "scroll_panel", "ScrollPanel", "Scroll Panel", "Containers",
            {}, {}, "Прокручиваемая панель DeltaQ",
            QSizeF(220.0, 160.0), {},
            {}, {}, {},
            true, true, false
        },
        {
            "tab_panel", "TabPanel", "Tab Panel", "Containers",
            {}, {}, "Панель вкладок DeltaQ",
            QSizeF(240.0, 180.0), {},
            {}, {}, {},
            true, true, false
        },
        {
            "group_box", "GroupBox", "Group Box", "Containers",
            {}, {}, "Группа элементов DeltaQ",
            QSizeF(200.0, 150.0), {},
            {
                {"text", QVariant(), true}
            },
            {}, {},
            true, true, false
        },
        {
            "button", "Button", "Button", "Input",
            "ui_module_button", "UI_Button", "Контракт кнопки DeltaQ",
            QSizeF(120.0, 40.0), "onClick",
            {
                {"text", QVariant(), true}
            },
            {
                {"text", "string", "\"Button\""},
                {"bg_color", "string", "\"#4CAF50\""},
                {"enabled", "bool", "1"}
            },
            {
                {"onClick", "signal", ""}
            },
            false, true, true
        },
        {
            "text_field", "TextField", "Text Field", "Input",
            "ui_module_textfield", "UI_TextField", "Контракт текстового поля DeltaQ",
            QSizeF(150.0, 30.0), "onTextChanged",
            {
                {"text", QString(), false},
                {"placeholder", QString(), false},
                {"max_length", 0, false},
                {"read_only", false, false},
                {"password", false, false}
            },
            {
                {"text", "string", "\"\""},
                {"placeholder", "string", "\"\""},
                {"enabled", "bool", "1"}
            },
            {
                {"onTextChanged", "signal", ""},
                {"value", "string", ""}
            },
            false, true, true
        },
        {
            "text_area", "TextArea", "Text Area", "Input",
            {}, {}, "Многострочное текстовое поле DeltaQ",
            QSizeF(180.0, 80.0), "onTextChanged",
            {
                {"text", QString(), false},
                {"placeholder", QString(), false},
                {"max_length", 0, false},
                {"read_only", false, false}
            },
            {}, {},
            false, true, false
        },
        {
            "checkbox", "Checkbox", "Checkbox", "Input",
            "ui_module_checkbox", "UI_Checkbox", "Контракт чекбокса DeltaQ",
            QSizeF(120.0, 28.0), "onToggled",
            {
                {"text", QVariant(), true},
                {"checked", false, false}
            },
            {
                {"text", "string", "\"Checkbox\""},
                {"checked", "bool", "0"}
            },
            {
                {"onToggled", "signal", ""},
                {"checked", "bool", ""}
            },
            false, true, true
        },
        {
            "radio_button", "RadioButton", "Radio Button", "Input",
            {}, {}, "Контракт радиокнопки DeltaQ",
            QSizeF(120.0, 28.0), "onToggled",
            {
                {"text", QVariant(), true},
                {"selected", false, false}
            },
            {}, {},
            false, true, false
        },
        {
            "combo_box", "ComboBox", "Combo Box", "Input",
            "ui_module_combobox", "UI_ComboBox", "Контракт выпадающего списка DeltaQ",
            QSizeF(150.0, 30.0), "onSelectionChanged",
            {
                {"items", QString(), false},
                {"selected", 0, false}
            },
            {
                {"items", "string", "\"\""},
                {"selected", "int", "0"}
            },
            {
                {"onSelectionChanged", "signal", ""},
                {"selected", "int", ""}
            },
            false, true, true
        },
        {
            "slider", "Slider", "Slider", "Input",
            "ui_module_slider", "UI_Slider", "Контракт слайдера DeltaQ",
            QSizeF(200.0, 30.0), "onValueChanged",
            {
                {"min", 0, false},
                {"max", 100, false},
                {"value", 50, false},
                {"step", 1, false}
            },
            {
                {"min", "int", "0"},
                {"max", "int", "100"},
                {"value", "int", "50"}
            },
            {
                {"onValueChanged", "signal", ""},
                {"value", "int", ""}
            },
            false, true, true
        },
        {
            "spin_box", "SpinBox", "Spin Box", "Input",
            {}, {}, "Контракт числового поля DeltaQ",
            QSizeF(120.0, 30.0), "onValueChanged",
            {
                {"min", 0, false},
                {"max", 100, false},
                {"value", 0, false}
            },
            {}, {},
            false, true, false
        },
        {
            "label", "Label", "Label", "Display",
            "ui_module_label", "UI_Label", "Контракт текстовой метки DeltaQ",
            QSizeF(120.0, 30.0), {},
            {
                {"text", QVariant(), true},
                {"alignment", QStringLiteral("left"), false},
                {"word_wrap", false, false}
            },
            {
                {"text", "string", "\"Label\""},
                {"font_color", "string", "\"#FFFFFF\""},
                {"alignment", "string", "\"left\""}
            },
            {},
            false, true, true
        },
        {
            "image", "Image", "Image", "Display",
            "ui_module_image", "UI_Image", "Контракт изображения DeltaQ",
            QSizeF(100.0, 100.0), {},
            {
                {"path", QString(), false}
            },
            {
                {"path", "string", "\"\""},
                {"width", "int", "100"},
                {"height", "int", "100"}
            },
            {},
            false, true, true
        },
        {
            "progress_bar", "ProgressBar", "Progress Bar", "Display",
            "ui_module_progressbar", "UI_ProgressBar", "Контракт индикатора прогресса DeltaQ",
            QSizeF(200.0, 24.0), {},
            {
                {"value", 40, false},
                {"min", 0, false},
                {"max", 100, false},
                {"show_text", true, false}
            },
            {
                {"value", "int", "0"},
                {"min", "int", "0"},
                {"max", "int", "100"}
            },
            {},
            false, true, true
        },
        {
            "canvas", "Canvas", "Canvas", "Display",
            {}, {}, "Холст пользовательской отрисовки DeltaQ",
            QSizeF(200.0, 140.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "table", "Table", "Table", "Display",
            {}, {}, "Табличный виджет DeltaQ",
            QSizeF(220.0, 140.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "list_view", "ListView", "List View", "Display",
            {}, {}, "Список DeltaQ",
            QSizeF(180.0, 140.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "tree_view", "TreeView", "Tree View", "Display",
            {}, {}, "Древовидный список DeltaQ",
            QSizeF(180.0, 160.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "menu_bar", "MenuBar", "Menu Bar", "Navigation",
            {}, {}, "Меню приложения DeltaQ",
            QSizeF(240.0, 28.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "tool_bar", "ToolBar", "Tool Bar", "Navigation",
            {}, {}, "Панель инструментов DeltaQ",
            QSizeF(240.0, 32.0), {},
            {}, {}, {},
            false, true, false
        },
        {
            "status_bar", "StatusBar", "Status Bar", "Navigation",
            {}, {}, "Строка состояния DeltaQ",
            QSizeF(240.0, 24.0), {},
            {}, {}, {},
            false, true, false
        }
    };

    return catalog;
}

// Ищет описание UI-контракта по любому известному идентификатору.
inline const UIContractSpec *findUIContractSpecByAny(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty())
        return nullptr;

    for (const auto &spec : uiContractCatalog()) {
        if (trimmed.compare(spec.contractType, Qt::CaseInsensitive) == 0
            || trimmed.compare(spec.legacyWidgetType, Qt::CaseInsensitive) == 0
            || (!spec.moduleId.isEmpty() && trimmed.compare(spec.moduleId, Qt::CaseInsensitive) == 0)
            || (!spec.moduleName.isEmpty() && trimmed.compare(spec.moduleName, Qt::CaseInsensitive) == 0)
            || trimmed.compare(spec.displayName, Qt::CaseInsensitive) == 0) {
            return &spec;
        }
    }

    return nullptr;
}

// Возвращает канонический тип контракта для legacy/display/module идентификатора.
inline QString canonicalUIContractType(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->contractType;
    return {};
}

// Возвращает legacy-тип виджета для отображения в дизайнере и .dqui.
inline QString legacyUIWidgetType(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->legacyWidgetType;
    return value;
}

// Возвращает отображаемое имя UI-контракта.
inline QString displayUIContractName(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->displayName;
    return value;
}

// Возвращает имя события по умолчанию для быстрого открытия обработчика.
inline QString defaultUIEventForContractType(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->defaultEvent.isEmpty() ? QStringLiteral("onClick") : spec->defaultEvent;
    return QStringLiteral("onClick");
}

// Возвращает стандартный размер виджета для drag preview и первичной вставки.
inline QSizeF defaultUIWidgetSize(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->defaultSize;
    return QSizeF(120.0, 40.0);
}

// Проверяет, является ли контракт контейнерным.
inline bool isContainerUIContractType(const QString &value)
{
    if (const auto *spec = findUIContractSpecByAny(value))
        return spec->isContainer;
    return false;
}

} // namespace DeltaQ

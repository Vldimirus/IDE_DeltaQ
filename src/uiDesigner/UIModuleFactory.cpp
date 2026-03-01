// Фабрика UI-модулей — реализация
#include "UIModuleFactory.h"
#include "../core/ModuleRegistry.h"

namespace DeltaQ {

QVector<Module> UIModuleFactory::createUIModules()
{
    return {
        createButton(),
        createTextField(),
        createLabel(),
        createSlider(),
        createCheckbox(),
        createProgressBar(),
        createImage(),
        createComboBox()
    };
}

void UIModuleFactory::registerAll(ModuleRegistry *registry)
{
    if (!registry) return;

    auto modules = createUIModules();
    for (const auto &mod : modules) {
        // Проверяем, не зарегистрирован ли уже
        if (!registry->findModule(mod.id))
            registry->registerModule(mod);
    }
}

bool UIModuleFactory::isUIModule(const QString &moduleId)
{
    // UI-модули имеют фиксированные ID с префиксом "ui_"
    return moduleId.startsWith("ui_module_");
}

bool UIModuleFactory::isUIModuleByName(const QString &moduleName)
{
    static QStringList uiNames = {
        "UI_Button", "UI_TextField", "UI_Label", "UI_Slider",
        "UI_Checkbox", "UI_ProgressBar", "UI_Image", "UI_ComboBox"
    };
    return uiNames.contains(moduleName);
}

Module UIModuleFactory::createButton()
{
    Module m;
    m.id = "ui_module_button";
    m.name = "UI_Button";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Кнопка — обрабатывает нажатие";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed"; // Фиксированные модули всегда 'passed'

    m.includes = {"SDL2/SDL.h", "SDL2/SDL_ttf.h"};
    m.sourceCode =
        "// UI_Button — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    const char *text;\n"
        "    SDL_Color bg_color;\n"
        "    int enabled;\n"
        "    int pressed;\n"
        "} DQ_Button;\n"
        "\n"
        "void dq_ui_button_render(SDL_Renderer *r, DQ_Button *btn) {\n"
        "    SDL_SetRenderDrawColor(r, btn->bg_color.r, btn->bg_color.g,\n"
        "                           btn->bg_color.b, btn->bg_color.a);\n"
        "    SDL_RenderFillRect(r, &btn->rect);\n"
        "}\n"
        "\n"
        "int dq_ui_button_handle_event(DQ_Button *btn, SDL_Event *e) {\n"
        "    if (!btn->enabled) return 0;\n"
        "    if (e->type == SDL_MOUSEBUTTONDOWN) {\n"
        "        SDL_Point p = {e->button.x, e->button.y};\n"
        "        if (SDL_PointInRect(&p, &btn->rect)) return 1; // onClick\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    m.inputs.append({"text", "string", "\"Button\""});
    m.inputs.append({"bg_color", "string", "\"#4CAF50\""});
    m.inputs.append({"enabled", "bool", "1"});

    m.outputs.append({"onClick", "signal", ""});

    return m;
}

Module UIModuleFactory::createTextField()
{
    Module m;
    m.id = "ui_module_textfield";
    m.name = "UI_TextField";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Текстовое поле ввода";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h", "SDL2/SDL_ttf.h"};
    m.sourceCode =
        "// UI_TextField — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    char text[256];\n"
        "    const char *placeholder;\n"
        "    int enabled;\n"
        "    int focused;\n"
        "    int cursor_pos;\n"
        "} DQ_TextField;\n"
        "\n"
        "void dq_ui_textfield_render(SDL_Renderer *r, DQ_TextField *tf) {\n"
        "    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);\n"
        "    SDL_RenderFillRect(r, &tf->rect);\n"
        "    SDL_SetRenderDrawColor(r, tf->focused ? 100 : 60,\n"
        "                           tf->focused ? 100 : 60, tf->focused ? 255 : 80, 255);\n"
        "    SDL_RenderDrawRect(r, &tf->rect);\n"
        "}\n"
        "\n"
        "int dq_ui_textfield_handle_event(DQ_TextField *tf, SDL_Event *e) {\n"
        "    if (!tf->enabled) return 0;\n"
        "    if (e->type == SDL_TEXTINPUT && tf->focused) {\n"
        "        strcat(tf->text, e->text.text);\n"
        "        return 1; // onTextChanged\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    m.inputs.append({"text", "string", "\"\""});
    m.inputs.append({"placeholder", "string", "\"\""});
    m.inputs.append({"enabled", "bool", "1"});

    m.outputs.append({"onTextChanged", "signal", ""});
    m.outputs.append({"value", "string", ""});

    return m;
}

Module UIModuleFactory::createLabel()
{
    Module m;
    m.id = "ui_module_label";
    m.name = "UI_Label";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Текстовая метка";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h", "SDL2/SDL_ttf.h"};
    m.sourceCode =
        "// UI_Label — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    const char *text;\n"
        "    SDL_Color font_color;\n"
        "    int alignment; // 0=left, 1=center, 2=right\n"
        "} DQ_Label;\n"
        "\n"
        "void dq_ui_label_render(SDL_Renderer *r, DQ_Label *lbl, TTF_Font *font) {\n"
        "    if (!lbl->text || !font) return;\n"
        "    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, lbl->text, lbl->font_color);\n"
        "    if (!surf) return;\n"
        "    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);\n"
        "    SDL_Rect dst = {lbl->rect.x, lbl->rect.y, surf->w, surf->h};\n"
        "    SDL_RenderCopy(r, tex, NULL, &dst);\n"
        "    SDL_DestroyTexture(tex);\n"
        "    SDL_FreeSurface(surf);\n"
        "}\n";

    m.inputs.append({"text", "string", "\"Label\""});
    m.inputs.append({"font_color", "string", "\"#FFFFFF\""});
    m.inputs.append({"alignment", "string", "\"left\""});

    // Нет выходных портов
    return m;
}

Module UIModuleFactory::createSlider()
{
    Module m;
    m.id = "ui_module_slider";
    m.name = "UI_Slider";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Ползунок (слайдер)";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h"};
    m.sourceCode =
        "// UI_Slider — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    int min_val, max_val, value;\n"
        "    int dragging;\n"
        "} DQ_Slider;\n"
        "\n"
        "void dq_ui_slider_render(SDL_Renderer *r, DQ_Slider *s) {\n"
        "    // Трек (фон)\n"
        "    SDL_SetRenderDrawColor(r, 60, 60, 60, 255);\n"
        "    SDL_Rect track = {s->rect.x, s->rect.y + s->rect.h/2 - 2,\n"
        "                      s->rect.w, 4};\n"
        "    SDL_RenderFillRect(r, &track);\n"
        "    // Ползунок\n"
        "    float ratio = (float)(s->value - s->min_val) / (s->max_val - s->min_val);\n"
        "    int knob_x = s->rect.x + (int)(ratio * s->rect.w);\n"
        "    SDL_Rect knob = {knob_x - 6, s->rect.y, 12, s->rect.h};\n"
        "    SDL_SetRenderDrawColor(r, 100, 150, 255, 255);\n"
        "    SDL_RenderFillRect(r, &knob);\n"
        "}\n"
        "\n"
        "int dq_ui_slider_handle_event(DQ_Slider *s, SDL_Event *e) {\n"
        "    if (e->type == SDL_MOUSEBUTTONDOWN) {\n"
        "        SDL_Point p = {e->button.x, e->button.y};\n"
        "        if (SDL_PointInRect(&p, &s->rect)) s->dragging = 1;\n"
        "    } else if (e->type == SDL_MOUSEBUTTONUP) {\n"
        "        s->dragging = 0;\n"
        "    } else if (e->type == SDL_MOUSEMOTION && s->dragging) {\n"
        "        float ratio = (float)(e->motion.x - s->rect.x) / s->rect.w;\n"
        "        if (ratio < 0) ratio = 0; if (ratio > 1) ratio = 1;\n"
        "        s->value = s->min_val + (int)(ratio * (s->max_val - s->min_val));\n"
        "        return 1; // onValueChanged\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    m.inputs.append({"min", "int", "0"});
    m.inputs.append({"max", "int", "100"});
    m.inputs.append({"value", "int", "50"});

    m.outputs.append({"onValueChanged", "signal", ""});
    m.outputs.append({"value", "int", ""});

    return m;
}

Module UIModuleFactory::createCheckbox()
{
    Module m;
    m.id = "ui_module_checkbox";
    m.name = "UI_Checkbox";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Флажок (чекбокс)";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h"};
    m.sourceCode =
        "// UI_Checkbox — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    const char *text;\n"
        "    int checked;\n"
        "} DQ_Checkbox;\n"
        "\n"
        "void dq_ui_checkbox_render(SDL_Renderer *r, DQ_Checkbox *cb) {\n"
        "    // Рамка\n"
        "    SDL_Rect box = {cb->rect.x, cb->rect.y, 18, 18};\n"
        "    SDL_SetRenderDrawColor(r, 180, 180, 180, 255);\n"
        "    SDL_RenderDrawRect(r, &box);\n"
        "    // Галочка\n"
        "    if (cb->checked) {\n"
        "        SDL_SetRenderDrawColor(r, 76, 175, 80, 255);\n"
        "        SDL_Rect inner = {box.x+3, box.y+3, 12, 12};\n"
        "        SDL_RenderFillRect(r, &inner);\n"
        "    }\n"
        "}\n"
        "\n"
        "int dq_ui_checkbox_handle_event(DQ_Checkbox *cb, SDL_Event *e) {\n"
        "    if (e->type == SDL_MOUSEBUTTONDOWN) {\n"
        "        SDL_Point p = {e->button.x, e->button.y};\n"
        "        if (SDL_PointInRect(&p, &cb->rect)) {\n"
        "            cb->checked = !cb->checked;\n"
        "            return 1; // onToggled\n"
        "        }\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    m.inputs.append({"text", "string", "\"Checkbox\""});
    m.inputs.append({"checked", "bool", "0"});

    m.outputs.append({"onToggled", "signal", ""});
    m.outputs.append({"checked", "bool", ""});

    return m;
}

Module UIModuleFactory::createProgressBar()
{
    Module m;
    m.id = "ui_module_progressbar";
    m.name = "UI_ProgressBar";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Индикатор прогресса";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h"};
    m.sourceCode =
        "// UI_ProgressBar — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    int value, min_val, max_val;\n"
        "} DQ_ProgressBar;\n"
        "\n"
        "void dq_ui_progressbar_render(SDL_Renderer *r, DQ_ProgressBar *pb) {\n"
        "    // Фон\n"
        "    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);\n"
        "    SDL_RenderFillRect(r, &pb->rect);\n"
        "    // Заполнение\n"
        "    float ratio = (float)(pb->value - pb->min_val) /\n"
        "                  (pb->max_val - pb->min_val);\n"
        "    if (ratio < 0) ratio = 0; if (ratio > 1) ratio = 1;\n"
        "    SDL_Rect fill = {pb->rect.x, pb->rect.y,\n"
        "                     (int)(pb->rect.w * ratio), pb->rect.h};\n"
        "    SDL_SetRenderDrawColor(r, 76, 175, 80, 255);\n"
        "    SDL_RenderFillRect(r, &fill);\n"
        "    // Рамка\n"
        "    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);\n"
        "    SDL_RenderDrawRect(r, &pb->rect);\n"
        "}\n";

    m.inputs.append({"value", "int", "0"});
    m.inputs.append({"min", "int", "0"});
    m.inputs.append({"max", "int", "100"});

    // Нет выходных портов
    return m;
}

Module UIModuleFactory::createImage()
{
    Module m;
    m.id = "ui_module_image";
    m.name = "UI_Image";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Виджет изображения";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h", "SDL2/SDL_image.h"};
    m.sourceCode =
        "// UI_Image — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    const char *path;\n"
        "    SDL_Texture *texture;\n"
        "} DQ_Image;\n"
        "\n"
        "void dq_ui_image_load(SDL_Renderer *r, DQ_Image *img) {\n"
        "    if (!img->path) return;\n"
        "    SDL_Surface *surf = IMG_Load(img->path);\n"
        "    if (!surf) return;\n"
        "    img->texture = SDL_CreateTextureFromSurface(r, surf);\n"
        "    SDL_FreeSurface(surf);\n"
        "}\n"
        "\n"
        "void dq_ui_image_render(SDL_Renderer *r, DQ_Image *img) {\n"
        "    if (img->texture)\n"
        "        SDL_RenderCopy(r, img->texture, NULL, &img->rect);\n"
        "}\n";

    m.inputs.append({"path", "string", "\"\""});
    m.inputs.append({"width", "int", "100"});
    m.inputs.append({"height", "int", "100"});

    // Нет выходных портов
    return m;
}

Module UIModuleFactory::createComboBox()
{
    Module m;
    m.id = "ui_module_combobox";
    m.name = "UI_ComboBox";
    m.version = "1.0.0";
    m.language = "c";
    m.description = "Выпадающий список";
    m.category = "ui";
    m.origin = "ui";
    m.testStatus = "passed";

    m.includes = {"SDL2/SDL.h"};
    m.sourceCode =
        "// UI_ComboBox — SDL2 реализация\n"
        "typedef struct {\n"
        "    SDL_Rect rect;\n"
        "    const char *items[32]; // Максимум 32 элемента\n"
        "    int item_count;\n"
        "    int selected;\n"
        "    int expanded;\n"
        "} DQ_ComboBox;\n"
        "\n"
        "void dq_ui_combobox_render(SDL_Renderer *r, DQ_ComboBox *cb) {\n"
        "    // Основная область\n"
        "    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);\n"
        "    SDL_RenderFillRect(r, &cb->rect);\n"
        "    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);\n"
        "    SDL_RenderDrawRect(r, &cb->rect);\n"
        "    // Стрелка раскрытия\n"
        "    int ax = cb->rect.x + cb->rect.w - 16;\n"
        "    int ay = cb->rect.y + cb->rect.h / 2;\n"
        "    SDL_RenderDrawLine(r, ax, ay-3, ax+5, ay+3);\n"
        "    SDL_RenderDrawLine(r, ax+5, ay+3, ax+10, ay-3);\n"
        "}\n"
        "\n"
        "int dq_ui_combobox_handle_event(DQ_ComboBox *cb, SDL_Event *e) {\n"
        "    if (e->type == SDL_MOUSEBUTTONDOWN) {\n"
        "        SDL_Point p = {e->button.x, e->button.y};\n"
        "        if (SDL_PointInRect(&p, &cb->rect)) {\n"
        "            cb->expanded = !cb->expanded;\n"
        "            return 1; // onSelectionChanged\n"
        "        }\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    m.inputs.append({"items", "string", "\"\""});
    m.inputs.append({"selected", "int", "0"});

    m.outputs.append({"onSelectionChanged", "signal", ""});
    m.outputs.append({"selected", "int", ""});

    return m;
}

} // namespace DeltaQ

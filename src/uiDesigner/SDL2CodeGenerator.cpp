// Генератор SDL2 C-кода — реализация
#include "SDL2CodeGenerator.h"
#include <deltaq/UILayout.h>
#include <QTextStream>
#include <QVector>

namespace DeltaQ {

GeneratedCode SDL2CodeGenerator::generate(const UILayout &layout, const QString &baseName)
{
    GeneratedCode code;
    code.baseName     = baseName;
    code.mainFile     = generateMainFile(layout, baseName);
    code.uiHeader     = generateUIHeader(layout, baseName);
    code.uiSource     = generateUISource(layout, baseName);
    code.eventsHeader = generateEventsHeader(layout, baseName);
    code.eventsSource = generateEventsSource(layout, baseName);
    return code;
}

// --- Вспомогательные ---

QString SDL2CodeGenerator::sanitizeName(const QString &name)
{
    QString result;
    for (QChar c : name) {
        if (c.isLetterOrNumber() || c == '_')
            result += c;
        else
            result += '_';
    }
    if (!result.isEmpty() && result[0].isDigit())
        result.prepend('_');
    return result;
}

QString SDL2CodeGenerator::widgetStructName(const QString &type)
{
    return "DQ_" + type;
}

QString SDL2CodeGenerator::widgetRenderFunc(const QString &type)
{
    return "render_" + type.toLower();
}

void SDL2CodeGenerator::collectWidgets(const UIWidget &widget, QVector<const UIWidget *> &out)
{
    if (widget.type != "Window")
        out.append(&widget);
    for (const auto &child : widget.children)
        collectWidgets(child, out);
}

void SDL2CodeGenerator::collectEvents(const UIWidget &widget, QMap<QString, QString> &out)
{
    for (auto it = widget.events.begin(); it != widget.events.end(); ++it)
        out[it.value()] = it.key(); // handler → event
    for (const auto &child : widget.children)
        collectEvents(child, out);
}

// --- main.c ---

QString SDL2CodeGenerator::generateMainFile(const UILayout &layout, const QString &baseName)
{
    QString s;
    QTextStream out(&s);

    out << "// Автогенерация DeltaQ IDE — main.c\n";
    out << "// Макет: " << layout.name << "\n\n";
    out << "#include <SDL2/SDL.h>\n";
    out << "#include <SDL2/SDL_ttf.h>\n";
    out << "#include <stdbool.h>\n";
    out << "#include \"" << baseName << ".h\"\n";
    out << "#include \"" << baseName << "_events.h\"\n\n";

    out << "int main(int argc, char *argv[]) {\n";
    out << "    (void)argc; (void)argv;\n\n";
    out << "    if (SDL_Init(SDL_INIT_VIDEO) < 0) {\n";
    out << "        SDL_Log(\"SDL_Init failed: %s\", SDL_GetError());\n";
    out << "        return 1;\n";
    out << "    }\n\n";
    out << "    if (TTF_Init() < 0) {\n";
    out << "        SDL_Log(\"TTF_Init failed: %s\", TTF_GetError());\n";
    out << "        SDL_Quit();\n";
    out << "        return 1;\n";
    out << "    }\n\n";

    int w = static_cast<int>(layout.window.geometry.width());
    int h = static_cast<int>(layout.window.geometry.height());

    out << "    SDL_Window *window = SDL_CreateWindow(\n";
    out << "        \"" << layout.name << "\",\n";
    out << "        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,\n";
    out << "        " << w << ", " << h << ",\n";
    out << "        SDL_WINDOW_SHOWN);\n";
    out << "    if (!window) {\n";
    out << "        SDL_Log(\"CreateWindow failed: %s\", SDL_GetError());\n";
    out << "        TTF_Quit();\n";
    out << "        SDL_Quit();\n";
    out << "        return 1;\n";
    out << "    }\n\n";

    out << "    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,\n";
    out << "        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);\n";
    out << "    if (!renderer) {\n";
    out << "        SDL_Log(\"CreateRenderer failed: %s\", SDL_GetError());\n";
    out << "        SDL_DestroyWindow(window);\n";
    out << "        TTF_Quit();\n";
    out << "        SDL_Quit();\n";
    out << "        return 1;\n";
    out << "    }\n\n";

    out << "    UIState ui;\n";
    out << "    ui_init(&ui);\n\n";

    out << "    bool running = true;\n";
    out << "    SDL_Event event;\n\n";

    out << "    while (running) {\n";
    out << "        while (SDL_PollEvent(&event)) {\n";
    out << "            if (event.type == SDL_QUIT) {\n";
    out << "                running = false;\n";
    out << "                break;\n";
    out << "            }\n";
    out << "            ui_handle_event(&ui, &event);\n";
    out << "        }\n\n";
    out << "        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);\n";
    out << "        SDL_RenderClear(renderer);\n";
    out << "        ui_render(&ui, renderer);\n";
    out << "        SDL_RenderPresent(renderer);\n";
    out << "    }\n\n";

    out << "    SDL_DestroyRenderer(renderer);\n";
    out << "    SDL_DestroyWindow(window);\n";
    out << "    TTF_Quit();\n";
    out << "    SDL_Quit();\n";
    out << "    return 0;\n";
    out << "}\n";

    return s;
}

// --- ui.h ---

QString SDL2CodeGenerator::generateUIHeader(const UILayout &layout, const QString &baseName)
{
    QVector<const UIWidget *> widgets;
    collectWidgets(layout.window, widgets);

    QString s;
    QTextStream out(&s);

    out << "// Автогенерация DeltaQ IDE — " << baseName << ".h\n";
    out << "#pragma once\n\n";
    out << "#include <SDL2/SDL.h>\n";
    out << "#include <SDL2/SDL_ttf.h>\n";
    out << "#include <stdbool.h>\n\n";

    // Структуры для каждого типа (дедупликация)
    QSet<QString> emittedTypes;
    for (const auto *w : widgets) {
        if (emittedTypes.contains(w->type)) continue;
        emittedTypes.insert(w->type);

        QString sn = widgetStructName(w->type);

        if (w->type == "Button") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    const char *text;\n";
            out << "    bool hovered;\n";
            out << "    bool pressed;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "Label") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    const char *text;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "TextField") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    char text[256];\n";
            out << "    int cursor_pos;\n";
            out << "    bool focused;\n";
            out << "    const char *placeholder;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "Checkbox") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    const char *text;\n";
            out << "    bool checked;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "Slider") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    int min_val, max_val, value;\n";
            out << "    bool dragging;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "ProgressBar") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    int min_val, max_val, value;\n";
            out << "    bool show_text;\n";
            out << "} " << sn << ";\n\n";
        } else if (w->type == "Panel") {
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "} " << sn << ";\n\n";
        } else {
            // Общая заглушка
            out << "typedef struct {\n";
            out << "    SDL_Rect rect;\n";
            out << "    /* TODO: " << w->type << " fields */\n";
            out << "} " << sn << ";\n\n";
        }
    }

    // UIState
    out << "typedef struct {\n";
    for (const auto *w : widgets) {
        QString sn = widgetStructName(w->type);
        QString varName = sanitizeName(w->name);
        out << "    " << sn << " " << varName << ";\n";
    }
    out << "} UIState;\n\n";

    // Прототипы
    out << "void ui_init(UIState *ui);\n";
    out << "void ui_render(UIState *ui, SDL_Renderer *renderer);\n";
    out << "void ui_handle_event(UIState *ui, SDL_Event *event);\n";

    return s;
}

// --- ui.c ---

QString SDL2CodeGenerator::generateUISource(const UILayout &layout, const QString &baseName)
{
    QVector<const UIWidget *> widgets;
    collectWidgets(layout.window, widgets);

    QString s;
    QTextStream out(&s);

    out << "// Автогенерация DeltaQ IDE — " << baseName << ".c\n";
    out << "#include \"" << baseName << ".h\"\n";
    out << "#include \"" << baseName << "_events.h\"\n";
    out << "#include <string.h>\n\n";

    // Функции рисования для каждого типа
    QSet<QString> emittedRenders;
    for (const auto *w : widgets) {
        if (emittedRenders.contains(w->type)) continue;
        emittedRenders.insert(w->type);

        QString sn = widgetStructName(w->type);
        QString fn = widgetRenderFunc(w->type);

        if (w->type == "Button") {
            out << "static void " << fn << "(" << sn << " *btn, SDL_Renderer *r) {\n";
            out << "    SDL_SetRenderDrawColor(r, btn->pressed ? 80 : (btn->hovered ? 70 : 61),\n";
            out << "                              btn->pressed ? 80 : (btn->hovered ? 70 : 61),\n";
            out << "                              btn->pressed ? 80 : (btn->hovered ? 70 : 61), 255);\n";
            out << "    SDL_RenderFillRect(r, &btn->rect);\n";
            out << "    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);\n";
            out << "    SDL_RenderDrawRect(r, &btn->rect);\n";
            out << "    /* TODO: render text with TTF */\n";
            out << "}\n\n";
        } else if (w->type == "Label") {
            out << "static void " << fn << "(" << sn << " *lbl, SDL_Renderer *r) {\n";
            out << "    (void)lbl; (void)r;\n";
            out << "    /* TODO: render text with TTF */\n";
            out << "}\n\n";
        } else if (w->type == "TextField") {
            out << "static void " << fn << "(" << sn << " *tf, SDL_Renderer *r) {\n";
            out << "    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);\n";
            out << "    SDL_RenderFillRect(r, &tf->rect);\n";
            out << "    SDL_SetRenderDrawColor(r, tf->focused ? 0 : 100,\n";
            out << "                              tf->focused ? 120 : 100,\n";
            out << "                              tf->focused ? 215 : 100, 255);\n";
            out << "    SDL_RenderDrawRect(r, &tf->rect);\n";
            out << "    /* TODO: render text with TTF */\n";
            out << "}\n\n";
        } else if (w->type == "Checkbox") {
            out << "static void " << fn << "(" << sn << " *cb, SDL_Renderer *r) {\n";
            out << "    SDL_Rect box = {cb->rect.x, cb->rect.y + (cb->rect.h - 16) / 2, 16, 16};\n";
            out << "    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);\n";
            out << "    SDL_RenderFillRect(r, &box);\n";
            out << "    SDL_SetRenderDrawColor(r, 120, 120, 120, 255);\n";
            out << "    SDL_RenderDrawRect(r, &box);\n";
            out << "    if (cb->checked) {\n";
            out << "        SDL_SetRenderDrawColor(r, 0, 180, 80, 255);\n";
            out << "        SDL_RenderDrawLine(r, box.x + 3, box.y + 8, box.x + 6, box.y + 12);\n";
            out << "        SDL_RenderDrawLine(r, box.x + 6, box.y + 12, box.x + 12, box.y + 3);\n";
            out << "    }\n";
            out << "    /* TODO: render text with TTF */\n";
            out << "}\n\n";
        } else if (w->type == "Slider") {
            out << "static void " << fn << "(" << sn << " *sl, SDL_Renderer *r) {\n";
            out << "    int track_y = sl->rect.y + sl->rect.h / 2;\n";
            out << "    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);\n";
            out << "    SDL_Rect track = {sl->rect.x + 8, track_y - 2, sl->rect.w - 16, 4};\n";
            out << "    SDL_RenderFillRect(r, &track);\n";
            out << "    int range = sl->max_val - sl->min_val;\n";
            out << "    int knob_x = sl->rect.x + 8;\n";
            out << "    if (range > 0)\n";
            out << "        knob_x += (sl->value - sl->min_val) * (sl->rect.w - 16) / range;\n";
            out << "    SDL_Rect knob = {knob_x - 6, track_y - 6, 12, 12};\n";
            out << "    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);\n";
            out << "    SDL_RenderFillRect(r, &knob);\n";
            out << "}\n\n";
        } else if (w->type == "ProgressBar") {
            out << "static void " << fn << "(" << sn << " *pb, SDL_Renderer *r) {\n";
            out << "    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);\n";
            out << "    SDL_RenderFillRect(r, &pb->rect);\n";
            out << "    int range = pb->max_val - pb->min_val;\n";
            out << "    if (range > 0) {\n";
            out << "        int fill_w = (pb->value - pb->min_val) * pb->rect.w / range;\n";
            out << "        SDL_Rect fill = {pb->rect.x, pb->rect.y, fill_w, pb->rect.h};\n";
            out << "        SDL_SetRenderDrawColor(r, 0, 120, 215, 255);\n";
            out << "        SDL_RenderFillRect(r, &fill);\n";
            out << "    }\n";
            out << "    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);\n";
            out << "    SDL_RenderDrawRect(r, &pb->rect);\n";
            out << "}\n\n";
        } else if (w->type == "Panel") {
            out << "static void " << fn << "(" << sn << " *pnl, SDL_Renderer *r) {\n";
            out << "    SDL_SetRenderDrawColor(r, 45, 45, 45, 255);\n";
            out << "    SDL_RenderFillRect(r, &pnl->rect);\n";
            out << "    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);\n";
            out << "    SDL_RenderDrawRect(r, &pnl->rect);\n";
            out << "}\n\n";
        } else {
            out << "static void " << fn << "(" << sn << " *w, SDL_Renderer *r) {\n";
            out << "    (void)w; (void)r;\n";
            out << "    /* TODO: implement " << w->type << " rendering */\n";
            out << "}\n\n";
        }
    }

    // ui_init
    out << "void ui_init(UIState *ui) {\n";
    out << "    memset(ui, 0, sizeof(UIState));\n";
    for (const auto *w : widgets) {
        QString varName = sanitizeName(w->name);
        int x = static_cast<int>(w->geometry.x());
        int y = static_cast<int>(w->geometry.y());
        int ww = static_cast<int>(w->geometry.width());
        int hh = static_cast<int>(w->geometry.height());
        out << "    ui->" << varName << ".rect = (SDL_Rect){" << x << ", " << y << ", " << ww << ", " << hh << "};\n";

        if (w->type == "Button" || w->type == "Label" || w->type == "Checkbox") {
            QString text = w->properties.value("text", w->name).toString();
            out << "    ui->" << varName << ".text = \"" << text << "\";\n";
        }
        if (w->type == "TextField") {
            QString placeholder = w->properties.value("placeholder", "").toString();
            out << "    ui->" << varName << ".placeholder = \"" << placeholder << "\";\n";
        }
        if (w->type == "Slider") {
            int minV = w->properties.value("min", 0).toInt();
            int maxV = w->properties.value("max", 100).toInt();
            int val = w->properties.value("value", 50).toInt();
            out << "    ui->" << varName << ".min_val = " << minV << ";\n";
            out << "    ui->" << varName << ".max_val = " << maxV << ";\n";
            out << "    ui->" << varName << ".value = " << val << ";\n";
        }
        if (w->type == "ProgressBar") {
            int minV = w->properties.value("min", 0).toInt();
            int maxV = w->properties.value("max", 100).toInt();
            int val = w->properties.value("value", 40).toInt();
            bool showText = w->properties.value("show_text", true).toBool();
            out << "    ui->" << varName << ".min_val = " << minV << ";\n";
            out << "    ui->" << varName << ".max_val = " << maxV << ";\n";
            out << "    ui->" << varName << ".value = " << val << ";\n";
            out << "    ui->" << varName << ".show_text = " << (showText ? "true" : "false") << ";\n";
        }
    }
    out << "}\n\n";

    // ui_render
    out << "void ui_render(UIState *ui, SDL_Renderer *renderer) {\n";
    for (const auto *w : widgets) {
        QString varName = sanitizeName(w->name);
        QString fn = widgetRenderFunc(w->type);
        out << "    " << fn << "(&ui->" << varName << ", renderer);\n";
    }
    out << "}\n\n";

    // ui_handle_event
    out << "void ui_handle_event(UIState *ui, SDL_Event *event) {\n";
    out << "    int mx, my;\n";
    out << "    if (event->type == SDL_MOUSEMOTION ||\n";
    out << "        event->type == SDL_MOUSEBUTTONDOWN ||\n";
    out << "        event->type == SDL_MOUSEBUTTONUP) {\n";
    out << "        mx = event->button.x;\n";
    out << "        my = event->button.y;\n";
    out << "    } else {\n";
    out << "        mx = my = 0;\n";
    out << "    }\n\n";

    for (const auto *w : widgets) {
        QString varName = sanitizeName(w->name);
        QString sn = widgetStructName(w->type);

        if (w->type == "Button") {
            out << "    // Button: " << w->name << "\n";
            out << "    {\n";
            out << "        SDL_Point p = {mx, my};\n";
            out << "        bool inside = SDL_PointInRect(&p, &ui->" << varName << ".rect);\n";
            out << "        ui->" << varName << ".hovered = inside;\n";
            out << "        if (event->type == SDL_MOUSEBUTTONDOWN && inside) {\n";
            out << "            ui->" << varName << ".pressed = true;\n";
            out << "        }\n";
            out << "        if (event->type == SDL_MOUSEBUTTONUP && ui->" << varName << ".pressed) {\n";
            out << "            ui->" << varName << ".pressed = false;\n";
            out << "            if (inside) {\n";

            // Обработчик события onClick
            if (w->events.contains("onClick")) {
                out << "                " << sanitizeName(w->events["onClick"]) << "();\n";
            } else {
                out << "                /* TODO: onClick handler */\n";
            }

            out << "            }\n";
            out << "        }\n";
            out << "    }\n\n";
        }
        else if (w->type == "Checkbox") {
            out << "    // Checkbox: " << w->name << "\n";
            out << "    if (event->type == SDL_MOUSEBUTTONDOWN) {\n";
            out << "        SDL_Point p = {mx, my};\n";
            out << "        if (SDL_PointInRect(&p, &ui->" << varName << ".rect))\n";
            out << "            ui->" << varName << ".checked = !ui->" << varName << ".checked;\n";
            out << "    }\n\n";
        }
        else if (w->type == "TextField") {
            out << "    // TextField: " << w->name << "\n";
            out << "    if (event->type == SDL_MOUSEBUTTONDOWN) {\n";
            out << "        SDL_Point p = {mx, my};\n";
            out << "        ui->" << varName << ".focused = SDL_PointInRect(&p, &ui->" << varName << ".rect);\n";
            out << "    }\n";
            out << "    if (event->type == SDL_TEXTINPUT && ui->" << varName << ".focused) {\n";
            out << "        int len = (int)strlen(ui->" << varName << ".text);\n";
            out << "        if (len < 255) {\n";
            out << "            strcat(ui->" << varName << ".text, event->text.text);\n";
            out << "        }\n";
            out << "    }\n";
            out << "    if (event->type == SDL_KEYDOWN && ui->" << varName << ".focused) {\n";
            out << "        if (event->key.keysym.sym == SDLK_BACKSPACE) {\n";
            out << "            int len = (int)strlen(ui->" << varName << ".text);\n";
            out << "            if (len > 0) ui->" << varName << ".text[len - 1] = '\\0';\n";
            out << "        }\n";
            out << "    }\n\n";
        }
        else if (w->type == "Slider") {
            out << "    // Slider: " << w->name << "\n";
            out << "    {\n";
            out << "        SDL_Point p = {mx, my};\n";
            out << "        bool inside = SDL_PointInRect(&p, &ui->" << varName << ".rect);\n";
            out << "        if (event->type == SDL_MOUSEBUTTONDOWN && inside)\n";
            out << "            ui->" << varName << ".dragging = true;\n";
            out << "        if (event->type == SDL_MOUSEBUTTONUP)\n";
            out << "            ui->" << varName << ".dragging = false;\n";
            out << "        if (ui->" << varName << ".dragging && event->type == SDL_MOUSEMOTION) {\n";
            out << "            int rel = mx - ui->" << varName << ".rect.x - 8;\n";
            out << "            int range = ui->" << varName << ".rect.w - 16;\n";
            out << "            if (range > 0) {\n";
            out << "                int val = ui->" << varName << ".min_val + rel * (ui->" << varName << ".max_val - ui->" << varName << ".min_val) / range;\n";
            out << "                if (val < ui->" << varName << ".min_val) val = ui->" << varName << ".min_val;\n";
            out << "                if (val > ui->" << varName << ".max_val) val = ui->" << varName << ".max_val;\n";
            out << "                ui->" << varName << ".value = val;\n";
            out << "            }\n";
            out << "        }\n";
            out << "    }\n\n";
        }
    }

    out << "}\n";
    return s;
}

// --- events.h ---

QString SDL2CodeGenerator::generateEventsHeader(const UILayout &layout, const QString &baseName)
{
    QMap<QString, QString> events;
    collectEvents(layout.window, events);

    QString s;
    QTextStream out(&s);

    out << "// Автогенерация DeltaQ IDE — " << baseName << "_events.h\n";
    out << "#pragma once\n\n";

    for (auto it = events.begin(); it != events.end(); ++it) {
        out << "void " << sanitizeName(it.key()) << "(void);\n";
    }

    if (events.isEmpty())
        out << "/* No event handlers defined */\n";

    return s;
}

// --- events.c ---

QString SDL2CodeGenerator::generateEventsSource(const UILayout &layout, const QString &baseName)
{
    QMap<QString, QString> events;
    collectEvents(layout.window, events);

    QString s;
    QTextStream out(&s);

    out << "// Автогенерация DeltaQ IDE — " << baseName << "_events.c\n";
    out << "#include \"" << baseName << "_events.h\"\n\n";

    for (auto it = events.begin(); it != events.end(); ++it) {
        out << "void " << sanitizeName(it.key()) << "(void) {\n";
        out << "    /* TODO: implement " << it.key() << " handler for event '" << it.value() << "' */\n";
        out << "}\n\n";
    }

    if (events.isEmpty())
        out << "/* No event handlers defined */\n";

    return s;
}

} // namespace DeltaQ

// Автогенерация DeltaQ IDE — window1.c
#include "window1.h"
#include "window1_events.h"
#include <string.h>

static void render_text(SDL_Renderer *r, TTF_Font *font,
                        const char *text, int x, int y, SDL_Color color) {
    if (!font || !text || !text[0]) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static void render_label(DQ_Label *lbl, SDL_Renderer *r, TTF_Font *font) {
    SDL_Color c = {200, 200, 200, 255};
    render_text(r, font, lbl->text, lbl->rect.x, lbl->rect.y + 5, c);
}

static void render_button(DQ_Button *btn, SDL_Renderer *r, TTF_Font *font) {
    SDL_SetRenderDrawColor(r, btn->pressed ? 80 : (btn->hovered ? 70 : 61),
                              btn->pressed ? 80 : (btn->hovered ? 70 : 61),
                              btn->pressed ? 80 : (btn->hovered ? 70 : 61), 255);
    SDL_RenderFillRect(r, &btn->rect);
    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
    SDL_RenderDrawRect(r, &btn->rect);
    SDL_Color c = {220, 220, 220, 255};
    render_text(r, font, btn->text, btn->rect.x + 8, btn->rect.y + 10, c);
}

static void render_textfield(DQ_TextField *tf, SDL_Renderer *r, TTF_Font *font) {
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_RenderFillRect(r, &tf->rect);
    SDL_SetRenderDrawColor(r, tf->focused ? 0 : 100,
                              tf->focused ? 120 : 100,
                              tf->focused ? 215 : 100, 255);
    SDL_RenderDrawRect(r, &tf->rect);
    const char *display = tf->text[0] ? tf->text : tf->placeholder;
    SDL_Color c = tf->text[0] ? (SDL_Color){220,220,220,255} : (SDL_Color){120,120,120,255};
    render_text(r, font, display, tf->rect.x + 4, tf->rect.y + 6, c);
}

static void render_textarea(DQ_TextArea *ta, SDL_Renderer *r, TTF_Font *font) {
    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
    SDL_RenderFillRect(r, &ta->rect);
    SDL_SetRenderDrawColor(r, 80, 80, 80, 255);
    SDL_RenderDrawRect(r, &ta->rect);
    if (!font || !ta->text[0]) return;
    int line_h = TTF_FontLineSkip(font);
    int y = ta->rect.y + 4 - ta->scroll_y;
    char *line_start = ta->text;
    while (*line_start) {
        char *nl = strchr(line_start, '\n');
        int len = nl ? (int)(nl - line_start) : (int)strlen(line_start);
        if (len > 0 && y + line_h > ta->rect.y && y < ta->rect.y + ta->rect.h) {
            char buf[512];
            int copy = len < 511 ? len : 511;
            memcpy(buf, line_start, copy);
            buf[copy] = '\0';
            SDL_Color c = {200, 200, 200, 255};
            render_text(r, font, buf, ta->rect.x + 4, y, c);
        }
        y += line_h;
        line_start += len + (nl ? 1 : 0);
        if (!nl) break;
    }
}

void ui_init(UIState *ui) {
    memset(ui, 0, sizeof(UIState));

    // Поиск системного шрифта
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        NULL
    };
    for (int i = 0; font_paths[i]; i++) {
        ui->font = TTF_OpenFont(font_paths[i], 14);
        if (ui->font) break;
    }

    ui->lblCounter.rect = (SDL_Rect){20, 20, 200, 30};
    strncpy(ui->lblCounter.text, "0", 63);
    ui->lblCounter.text[63] = '\0';
    ui->btnCopyToLog.rect = (SDL_Rect){20, 60, 180, 36};
    ui->btnCopyToLog.text = "Copy to Log";
    ui->btnInsertText.rect = (SDL_Rect){220, 60, 180, 36};
    ui->btnInsertText.text = "Text to Log";
    ui->editInput.rect = (SDL_Rect){20, 110, 380, 32};
    ui->editInput.placeholder = "Enter text...";
    ui->txtLog.rect = (SDL_Rect){20, 160, 600, 300};
    memset(ui->txtLog.text, 0, sizeof(ui->txtLog.text));
    ui->txtLog.scroll_y = 0;
}

void ui_render(UIState *ui, SDL_Renderer *renderer) {
    render_label(&ui->lblCounter, renderer, ui->font);
    render_button(&ui->btnCopyToLog, renderer, ui->font);
    render_button(&ui->btnInsertText, renderer, ui->font);
    render_textfield(&ui->editInput, renderer, ui->font);
    render_textarea(&ui->txtLog, renderer, ui->font);
}

void ui_handle_event(UIState *ui, SDL_Event *event) {
    int mx, my;
    if (event->type == SDL_MOUSEMOTION ||
        event->type == SDL_MOUSEBUTTONDOWN ||
        event->type == SDL_MOUSEBUTTONUP) {
        mx = event->button.x;
        my = event->button.y;
    } else {
        mx = my = 0;
    }

    // Button: btnCopyToLog
    {
        SDL_Point p = {mx, my};
        bool inside = SDL_PointInRect(&p, &ui->btnCopyToLog.rect);
        ui->btnCopyToLog.hovered = inside;
        if (event->type == SDL_MOUSEBUTTONDOWN && inside) {
            ui->btnCopyToLog.pressed = true;
        }
        if (event->type == SDL_MOUSEBUTTONUP && ui->btnCopyToLog.pressed) {
            ui->btnCopyToLog.pressed = false;
            if (inside) {
                on_btnCopyToLog_click(ui);
            }
        }
    }

    // Button: btnInsertText
    {
        SDL_Point p = {mx, my};
        bool inside = SDL_PointInRect(&p, &ui->btnInsertText.rect);
        ui->btnInsertText.hovered = inside;
        if (event->type == SDL_MOUSEBUTTONDOWN && inside) {
            ui->btnInsertText.pressed = true;
        }
        if (event->type == SDL_MOUSEBUTTONUP && ui->btnInsertText.pressed) {
            ui->btnInsertText.pressed = false;
            if (inside) {
                on_btnInsertText_click(ui);
            }
        }
    }

    // TextField: editInput
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        SDL_Point p = {mx, my};
        ui->editInput.focused = SDL_PointInRect(&p, &ui->editInput.rect);
    }
    if (event->type == SDL_TEXTINPUT && ui->editInput.focused) {
        int len = (int)strlen(ui->editInput.text);
        if (len < 255) {
            strcat(ui->editInput.text, event->text.text);
        }
    }
    if (event->type == SDL_KEYDOWN && ui->editInput.focused) {
        if (event->key.keysym.sym == SDLK_BACKSPACE) {
            int len = (int)strlen(ui->editInput.text);
            if (len > 0) ui->editInput.text[len - 1] = '\0';
        }
    }

    // TextArea: txtLog — прокрутка колесом
    if (event->type == SDL_MOUSEWHEEL) {
        SDL_Point p; SDL_GetMouseState(&p.x, &p.y);
        if (SDL_PointInRect(&p, &ui->txtLog.rect)) {
            ui->txtLog.scroll_y -= event->wheel.y * 20;
            if (ui->txtLog.scroll_y < 0) ui->txtLog.scroll_y = 0;
        }
    }

}

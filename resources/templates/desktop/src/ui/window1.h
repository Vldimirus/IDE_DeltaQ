// Автогенерация DeltaQ IDE — window1.h
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

typedef struct {
    SDL_Rect rect;
    char text[64];
} DQ_Label;

typedef struct {
    SDL_Rect rect;
    const char *text;
    bool hovered;
    bool pressed;
} DQ_Button;

typedef struct {
    SDL_Rect rect;
    char text[256];
    int cursor_pos;
    bool focused;
    const char *placeholder;
} DQ_TextField;

typedef struct {
    SDL_Rect rect;
    char text[4096];
    int scroll_y;
} DQ_TextArea;

typedef struct {
    TTF_Font *font;
    DQ_Label lblCounter;
    DQ_Button btnCopyToLog;
    DQ_Button btnInsertText;
    DQ_TextField editInput;
    DQ_TextArea txtLog;
} UIState;

void ui_init(UIState *ui);
void ui_render(UIState *ui, SDL_Renderer *renderer);
void ui_handle_event(UIState *ui, SDL_Event *event);

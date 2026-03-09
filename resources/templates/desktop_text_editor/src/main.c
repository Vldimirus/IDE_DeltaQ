#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum MenuId {
    MENU_NONE = 0,
    MENU_FILE = 1,
    MENU_EDIT = 2,
    MENU_HELP = 3
};

enum ActionId {
    ACTION_NONE = 0,
    ACTION_NEW,
    ACTION_QUIT,
    ACTION_CLEAR,
    ACTION_INSERT_SAMPLE,
    ACTION_TOGGLE_ABOUT
};

typedef struct MenuButton {
    const char *label;
    SDL_Rect rect;
    int menu_id;
} MenuButton;

typedef struct AppState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    bool running;
    bool show_about;
    int open_menu;
    int window_width;
    int window_height;
    int scroll_line;
    size_t text_length;
    char text[32768];
    MenuButton menu_buttons[3];
} AppState;

static SDL_Color rgb(unsigned char r, unsigned char g, unsigned char b)
{
    SDL_Color color = {r, g, b, 255};
    return color;
}

static void fill_rect(SDL_Renderer *renderer, const SDL_Rect *rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, rect);
}

static void stroke_rect(SDL_Renderer *renderer, const SDL_Rect *rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, rect);
}

static bool point_in_rect(int x, int y, const SDL_Rect *rect)
{
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

static TTF_Font *open_default_font(int size)
{
    static const char *paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };

    size_t i;
    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        TTF_Font *font = TTF_OpenFont(paths[i], size);
        if (font)
            return font;
    }
    return NULL;
}

static bool draw_text(SDL_Renderer *renderer, TTF_Font *font, const char *text,
                      int x, int y, SDL_Color color)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect rect;

    if (!font || !text || !text[0])
        return true;

    surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface)
        return false;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return false;
    }

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    return true;
}

static void append_text(AppState *app, const char *text)
{
    const size_t incoming = strlen(text);
    const size_t available = sizeof(app->text) - app->text_length - 1U;
    const size_t chunk = incoming < available ? incoming : available;

    if (chunk == 0U)
        return;

    memcpy(app->text + app->text_length, text, chunk);
    app->text_length += chunk;
    app->text[app->text_length] = '\0';
}

static void append_char(AppState *app, char ch)
{
    if (app->text_length + 1U >= sizeof(app->text))
        return;

    app->text[app->text_length++] = ch;
    app->text[app->text_length] = '\0';
}

static void erase_last_char(AppState *app)
{
    if (app->text_length == 0U)
        return;

    --app->text_length;
    app->text[app->text_length] = '\0';
}

static void clear_text(AppState *app)
{
    app->text[0] = '\0';
    app->text_length = 0U;
    app->scroll_line = 0;
}

static void insert_sample_text(AppState *app)
{
    append_text(app,
                "DeltaQ desktop text editor template\n"
                "-----------------------------------\n"
                "Type into the editor area.\n"
                "Use Ctrl+N to clear and Ctrl+Q to quit.\n");
}

static SDL_Rect editor_rect(const AppState *app)
{
    SDL_Rect rect;
    rect.x = 24;
    rect.y = 56;
    rect.w = app->window_width - 48;
    rect.h = app->window_height - 104;
    return rect;
}

static SDL_Rect dropdown_rect(int menu_id)
{
    SDL_Rect rect;
    rect.y = 32;
    rect.w = 180;
    rect.h = 70;

    if (menu_id == MENU_FILE)
        rect.x = 12;
    else if (menu_id == MENU_EDIT)
        rect.x = 96;
    else
        rect.x = 180;

    return rect;
}

static void setup_menu_buttons(AppState *app)
{
    app->menu_buttons[0].label = "File";
    app->menu_buttons[0].rect = (SDL_Rect) {12, 4, 72, 24};
    app->menu_buttons[0].menu_id = MENU_FILE;

    app->menu_buttons[1].label = "Edit";
    app->menu_buttons[1].rect = (SDL_Rect) {96, 4, 72, 24};
    app->menu_buttons[1].menu_id = MENU_EDIT;

    app->menu_buttons[2].label = "Help";
    app->menu_buttons[2].rect = (SDL_Rect) {180, 4, 72, 24};
    app->menu_buttons[2].menu_id = MENU_HELP;
}

static int menu_action_at(const AppState *app, int mouse_x, int mouse_y)
{
    int index;
    SDL_Rect rect;

    if (app->open_menu == MENU_NONE)
        return ACTION_NONE;

    rect = dropdown_rect(app->open_menu);
    if (!point_in_rect(mouse_x, mouse_y, &rect))
        return ACTION_NONE;

    index = (mouse_y - rect.y) / 22;
    if (app->open_menu == MENU_FILE)
        return index == 0 ? ACTION_NEW : ACTION_QUIT;
    if (app->open_menu == MENU_EDIT)
        return index == 0 ? ACTION_CLEAR : ACTION_INSERT_SAMPLE;
    if (app->open_menu == MENU_HELP)
        return ACTION_TOGGLE_ABOUT;
    return ACTION_NONE;
}

static void run_action(AppState *app, int action)
{
    switch (action) {
    case ACTION_NEW:
    case ACTION_CLEAR:
        clear_text(app);
        break;
    case ACTION_INSERT_SAMPLE:
        insert_sample_text(app);
        break;
    case ACTION_TOGGLE_ABOUT:
        app->show_about = !app->show_about;
        break;
    case ACTION_QUIT:
        app->running = false;
        break;
    default:
        break;
    }

    if (action != ACTION_TOGGLE_ABOUT)
        app->open_menu = MENU_NONE;
}

static void render_menu(AppState *app)
{
    int i;
    SDL_Rect bar = {0, 0, app->window_width, 32};
    fill_rect(app->renderer, &bar, rgb(235, 237, 240));

    for (i = 0; i < 3; ++i) {
        SDL_Color fg = rgb(40, 44, 52);
        if (app->open_menu == app->menu_buttons[i].menu_id) {
            fill_rect(app->renderer, &app->menu_buttons[i].rect, rgb(210, 219, 255));
            stroke_rect(app->renderer, &app->menu_buttons[i].rect, rgb(140, 160, 220));
        }
        draw_text(app->renderer, app->font, app->menu_buttons[i].label,
                  app->menu_buttons[i].rect.x + 10, app->menu_buttons[i].rect.y + 4, fg);
    }
}

static void render_dropdown(AppState *app)
{
    SDL_Rect rect;
    SDL_Color bg = rgb(250, 250, 252);
    SDL_Color fg = rgb(32, 35, 41);

    if (app->open_menu == MENU_NONE)
        return;

    rect = dropdown_rect(app->open_menu);
    fill_rect(app->renderer, &rect, bg);
    stroke_rect(app->renderer, &rect, rgb(160, 168, 178));

    if (app->open_menu == MENU_FILE) {
        draw_text(app->renderer, app->font, "New", rect.x + 12, rect.y + 4, fg);
        draw_text(app->renderer, app->font, "Quit", rect.x + 12, rect.y + 26, fg);
    } else if (app->open_menu == MENU_EDIT) {
        draw_text(app->renderer, app->font, "Clear", rect.x + 12, rect.y + 4, fg);
        draw_text(app->renderer, app->font, "Insert Sample", rect.x + 12, rect.y + 26, fg);
    } else if (app->open_menu == MENU_HELP) {
        draw_text(app->renderer, app->font, "About", rect.x + 12, rect.y + 4, fg);
    }
}

static void render_editor_text(AppState *app, const SDL_Rect *rect)
{
    const int line_height = TTF_FontHeight(app->font) + 4;
    const char *cursor = app->text;
    char line[1024];
    int line_index = 0;
    int draw_y = rect->y + 10;
    int line_length = 0;

    while (1) {
        const char ch = *cursor;
        const bool flush = ch == '\n' || ch == '\0' || line_length >= (int) sizeof(line) - 1;
        if (!flush) {
            line[line_length++] = ch;
            ++cursor;
            continue;
        }

        line[line_length] = '\0';
        if (line_index >= app->scroll_line && draw_y + line_height < rect->y + rect->h) {
            draw_text(app->renderer, app->font, line, rect->x + 12, draw_y, rgb(40, 44, 52));
            draw_y += line_height;
        }

        line_length = 0;
        ++line_index;

        if (ch == '\0')
            break;
        ++cursor;
    }
}

static void render_about_overlay(AppState *app)
{
    SDL_Rect panel;

    if (!app->show_about)
        return;

    panel.x = app->window_width / 2 - 200;
    panel.y = app->window_height / 2 - 90;
    panel.w = 400;
    panel.h = 180;

    fill_rect(app->renderer, &panel, rgb(250, 250, 252));
    stroke_rect(app->renderer, &panel, rgb(100, 110, 125));
    draw_text(app->renderer, app->font, "__PROJECT_NAME__", panel.x + 20, panel.y + 22, rgb(20, 24, 28));
    draw_text(app->renderer, app->font, "SDL2 text editor template", panel.x + 20, panel.y + 52, rgb(60, 66, 74));
    draw_text(app->renderer, app->font, "Click menu entries or type directly.", panel.x + 20, panel.y + 82, rgb(60, 66, 74));
    draw_text(app->renderer, app->font, "Esc closes this panel.", panel.x + 20, panel.y + 112, rgb(60, 66, 74));
}

static void render(AppState *app)
{
    SDL_Rect rect;
    SDL_Rect status_bar;

    SDL_SetRenderDrawColor(app->renderer, 244, 246, 248, 255);
    SDL_RenderClear(app->renderer);

    render_menu(app);
    render_dropdown(app);

    rect = editor_rect(app);
    fill_rect(app->renderer, &rect, rgb(255, 255, 255));
    stroke_rect(app->renderer, &rect, rgb(188, 192, 198));
    render_editor_text(app, &rect);

    status_bar = (SDL_Rect) {0, app->window_height - 36, app->window_width, 36};
    fill_rect(app->renderer, &status_bar, rgb(235, 237, 240));
    draw_text(app->renderer, app->font,
              "Ctrl+N clear   Ctrl+Q quit   Mouse menu on top   Wheel: use arrow keys to scroll",
              12, app->window_height - 28, rgb(70, 78, 88));

    render_about_overlay(app);
    SDL_RenderPresent(app->renderer);
}

static bool init_app(AppState *app)
{
    memset(app, 0, sizeof(*app));
    app->running = true;
    app->window_width = 980;
    app->window_height = 680;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return false;
    }

    app->window = SDL_CreateWindow("__PROJECT_NAME__",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   app->window_width,
                                   app->window_height,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!app->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    app->font = open_default_font(17);
    if (!app->font) {
        fprintf(stderr, "No suitable TTF font was found.\n");
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    setup_menu_buttons(app);
    insert_sample_text(app);
    SDL_StartTextInput();
    return true;
}

static void shutdown_app(AppState *app)
{
    SDL_StopTextInput();
    if (app->font)
        TTF_CloseFont(app->font);
    if (app->renderer)
        SDL_DestroyRenderer(app->renderer);
    if (app->window)
        SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

static void handle_keydown(AppState *app, const SDL_KeyboardEvent *event)
{
    const bool ctrl = (event->keysym.mod & KMOD_CTRL) != 0;

    if (event->keysym.sym == SDLK_ESCAPE) {
        app->open_menu = MENU_NONE;
        app->show_about = false;
        return;
    }

    if (ctrl && event->keysym.sym == SDLK_n) {
        clear_text(app);
        return;
    }

    if (ctrl && event->keysym.sym == SDLK_q) {
        app->running = false;
        return;
    }

    if (event->keysym.sym == SDLK_BACKSPACE) {
        erase_last_char(app);
        return;
    }

    if (event->keysym.sym == SDLK_RETURN) {
        append_char(app, '\n');
        return;
    }

    if (event->keysym.sym == SDLK_UP && app->scroll_line > 0) {
        --app->scroll_line;
        return;
    }

    if (event->keysym.sym == SDLK_DOWN) {
        ++app->scroll_line;
        return;
    }
}

static void handle_mouse_down(AppState *app, int mouse_x, int mouse_y)
{
    int i;

    for (i = 0; i < 3; ++i) {
        if (point_in_rect(mouse_x, mouse_y, &app->menu_buttons[i].rect)) {
            if (app->open_menu == app->menu_buttons[i].menu_id)
                app->open_menu = MENU_NONE;
            else
                app->open_menu = app->menu_buttons[i].menu_id;
            return;
        }
    }

    run_action(app, menu_action_at(app, mouse_x, mouse_y));
    if (app->open_menu != MENU_NONE)
        app->open_menu = MENU_NONE;
}

int main(void)
{
    AppState app;

    if (!init_app(&app))
        return 1;

    while (app.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                app.running = false;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    app.window_width = event.window.data1;
                    app.window_height = event.window.data2;
                }
                break;
            case SDL_TEXTINPUT:
                append_text(&app, event.text.text);
                break;
            case SDL_KEYDOWN:
                handle_keydown(&app, &event.key);
                break;
            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_down(&app, event.button.x, event.button.y);
                break;
            default:
                break;
            }
        }

        render(&app);
    }

    shutdown_app(&app);
    return 0;
}

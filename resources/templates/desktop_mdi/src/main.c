#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct ChildWindow {
    const char *title;
    SDL_Rect frame;
    SDL_Color title_color;
    SDL_Color body_color;
} ChildWindow;

typedef struct AppState {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    bool running;
    int width;
    int height;
    int active_window;
    int drag_offset_x;
    int drag_offset_y;
    ChildWindow windows[3];
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
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
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

static SDL_Rect workspace_rect(const AppState *app)
{
    SDL_Rect rect;
    rect.x = 18;
    rect.y = 46;
    rect.w = app->width - 36;
    rect.h = app->height - 64;
    return rect;
}

static SDL_Rect title_bar_rect(const ChildWindow *window)
{
    SDL_Rect rect = window->frame;
    rect.h = 30;
    return rect;
}

static void clamp_window_to_workspace(ChildWindow *window, const SDL_Rect *workspace)
{
    if (window->frame.x < workspace->x)
        window->frame.x = workspace->x;
    if (window->frame.y < workspace->y)
        window->frame.y = workspace->y;
    if (window->frame.x + window->frame.w > workspace->x + workspace->w)
        window->frame.x = workspace->x + workspace->w - window->frame.w;
    if (window->frame.y + window->frame.h > workspace->y + workspace->h)
        window->frame.y = workspace->y + workspace->h - window->frame.h;
}

static void bring_to_front(AppState *app, int index)
{
    int i;
    ChildWindow selected;

    if (index < 0 || index >= 3)
        return;
    if (index == 2) {
        app->active_window = 2;
        return;
    }

    selected = app->windows[index];
    for (i = index; i < 2; ++i)
        app->windows[i] = app->windows[i + 1];
    app->windows[2] = selected;
    app->active_window = 2;
}

static void setup_windows(AppState *app)
{
    app->windows[0].title = "Inspector";
    app->windows[0].frame = (SDL_Rect) {90, 110, 280, 220};
    app->windows[0].title_color = rgb(74, 109, 167);
    app->windows[0].body_color = rgb(235, 241, 248);

    app->windows[1].title = "Editor";
    app->windows[1].frame = (SDL_Rect) {240, 170, 420, 280};
    app->windows[1].title_color = rgb(71, 151, 131);
    app->windows[1].body_color = rgb(236, 247, 243);

    app->windows[2].title = "Log";
    app->windows[2].frame = (SDL_Rect) {520, 94, 320, 210};
    app->windows[2].title_color = rgb(158, 96, 64);
    app->windows[2].body_color = rgb(248, 240, 235);
}

static void render_workspace(AppState *app)
{
    int i;
    SDL_Rect workspace = workspace_rect(app);
    SDL_Rect menu_bar = {0, 0, app->width, 34};

    fill_rect(app->renderer, &menu_bar, rgb(234, 236, 239));
    draw_text(app->renderer, app->font, "File", 14, 8, rgb(36, 40, 46));
    draw_text(app->renderer, app->font, "Window", 74, 8, rgb(36, 40, 46));
    draw_text(app->renderer, app->font, "Help", 156, 8, rgb(36, 40, 46));

    fill_rect(app->renderer, &workspace, rgb(214, 220, 228));
    stroke_rect(app->renderer, &workspace, rgb(136, 146, 160));
    draw_text(app->renderer, app->font,
              "Drag child windows by their title bars",
              workspace.x + 12, workspace.y + 10, rgb(67, 73, 82));

    for (i = 0; i < 3; ++i) {
        SDL_Rect shadow = app->windows[i].frame;
        SDL_Rect title_bar = title_bar_rect(&app->windows[i]);
        SDL_Rect body = app->windows[i].frame;

        shadow.x += 6;
        shadow.y += 6;
        fill_rect(app->renderer, &shadow, rgb(170, 178, 188));

        fill_rect(app->renderer, &body, app->windows[i].body_color);
        stroke_rect(app->renderer, &body, rgb(92, 100, 110));
        fill_rect(app->renderer, &title_bar, app->windows[i].title_color);

        draw_text(app->renderer, app->font, app->windows[i].title,
                  title_bar.x + 10, title_bar.y + 6, rgb(255, 255, 255));

        if (strcmp(app->windows[i].title, "Inspector") == 0) {
            draw_text(app->renderer, app->font, "Properties", body.x + 12, body.y + 46, rgb(44, 48, 56));
            draw_text(app->renderer, app->font, "Position: (120, 64)", body.x + 12, body.y + 74, rgb(44, 48, 56));
            draw_text(app->renderer, app->font, "Size: 420 x 280", body.x + 12, body.y + 102, rgb(44, 48, 56));
        } else if (strcmp(app->windows[i].title, "Editor") == 0) {
            draw_text(app->renderer, app->font, "Workspace host for nested tools", body.x + 12, body.y + 46, rgb(44, 48, 56));
            draw_text(app->renderer, app->font, "This is the main child area.", body.x + 12, body.y + 74, rgb(44, 48, 56));
        } else {
            draw_text(app->renderer, app->font, "[11:42] Window moved", body.x + 12, body.y + 46, rgb(44, 48, 56));
            draw_text(app->renderer, app->font, "[11:43] Workspace ready", body.x + 12, body.y + 74, rgb(44, 48, 56));
        }
    }
}

static bool init_app(AppState *app)
{
    memset(app, 0, sizeof(*app));
    app->running = true;
    app->width = 1120;
    app->height = 720;
    app->active_window = -1;

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
                                   app->width,
                                   app->height,
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

    setup_windows(app);
    return true;
}

static void shutdown_app(AppState *app)
{
    if (app->font)
        TTF_CloseFont(app->font);
    if (app->renderer)
        SDL_DestroyRenderer(app->renderer);
    if (app->window)
        SDL_DestroyWindow(app->window);
    TTF_Quit();
    SDL_Quit();
}

static void handle_mouse_down(AppState *app, int mouse_x, int mouse_y)
{
    int i;

    for (i = 2; i >= 0; --i) {
        SDL_Rect title_bar = title_bar_rect(&app->windows[i]);
        if (point_in_rect(mouse_x, mouse_y, &title_bar)) {
            bring_to_front(app, i);
            app->drag_offset_x = mouse_x - app->windows[2].frame.x;
            app->drag_offset_y = mouse_y - app->windows[2].frame.y;
            return;
        }
    }

    app->active_window = -1;
}

static void handle_mouse_motion(AppState *app, int mouse_x, int mouse_y)
{
    SDL_Rect workspace;

    if (app->active_window != 2)
        return;

    workspace = workspace_rect(app);
    app->windows[2].frame.x = mouse_x - app->drag_offset_x;
    app->windows[2].frame.y = mouse_y - app->drag_offset_y;
    clamp_window_to_workspace(&app->windows[2], &workspace);
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
                    app.width = event.window.data1;
                    app.height = event.window.data2;
                    SDL_Rect workspace = workspace_rect(&app);
                    clamp_window_to_workspace(&app.windows[0], &workspace);
                    clamp_window_to_workspace(&app.windows[1], &workspace);
                    clamp_window_to_workspace(&app.windows[2], &workspace);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                handle_mouse_down(&app, event.button.x, event.button.y);
                break;
            case SDL_MOUSEBUTTONUP:
                app.active_window = -1;
                break;
            case SDL_MOUSEMOTION:
                handle_mouse_motion(&app, event.motion.x, event.motion.y);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    app.running = false;
                break;
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 244, 245, 247, 255);
        SDL_RenderClear(app.renderer);
        render_workspace(&app);
        SDL_RenderPresent(app.renderer);
    }

    shutdown_app(&app);
    return 0;
}

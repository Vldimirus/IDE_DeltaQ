// Автогенерация DeltaQ IDE — main.c
// Макет: window1

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "ui/window1.h"
#include "ui/window1_events.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "window1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    UIState ui;
    ui_init(&ui);

    bool running = true;
    SDL_Event event;
    Uint32 last_tick = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            ui_handle_event(&ui, &event);
        }

        Uint32 now = SDL_GetTicks();
        ui_update(&ui, now - last_tick);
        last_tick = now;

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ui_render(&ui, renderer);
        SDL_RenderPresent(renderer);
    }

    if (ui.font) TTF_CloseFont(ui.font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

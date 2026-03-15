// SQLite Notes Desktop — window1_events.c
#include "window1_events.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Uint32 total_ms = 0;
static int autoclose_ms = -2;
static int ui_bootstrapped = 0;

/* Копирует текст в UI-буфер безопасно и всегда оставляет завершающий `\0`. */
static void copy_text(char *dst, size_t size, const char *src)
{
    if (!dst || size == 0U)
        return;
    if (!src)
        src = "";

    strncpy(dst, src, size - 1U);
    dst[size - 1U] = '\0';
}

/* Читает timeout для headless auto-close из env и валидирует разумный диапазон. */
static int read_autoclose_ms(void)
{
    const char *env = getenv("DQ_SQLITE_NOTES_DESKTOP_AUTOCLOSE_MS");
    if (!env || !*env)
        return -1;

    {
        const long value = strtol(env, NULL, 10);
        if (value <= 0 || value > 600000)
            return -1;
        return (int) value;
    }
}

/* Заполняет desktop surface статическим пояснением для SQLite-backed startup flow. */
static void bootstrap_ui(UIState *ui)
{
    static const char initial_text[] =
        "SQLite startup flow completed before the event loop.\n"
        "\n"
        "The graph prepared runtime/notes.db, created the notes table,\n"
        "stored one persisted note, then loaded it back and printed the\n"
        "value to stdout as a deterministic regression proof.\n";

    copy_text(ui->txtNote.text, sizeof(ui->txtNote.text), initial_text);
    copy_text(ui->lblStatus.text, sizeof(ui->lblStatus.text), "SQLite desktop sample is running");
}

/* Обновляет UI один раз на старте и завершает приложение по auto-close timeout. */
void ui_update(UIState *ui, Uint32 delta_ms)
{
    if (!ui_bootstrapped) {
        ui_bootstrapped = 1;
        bootstrap_ui(ui);
    }

    if (autoclose_ms == -2)
        autoclose_ms = read_autoclose_ms();

    total_ms += delta_ms;
    if (autoclose_ms > 0 && total_ms >= (Uint32) autoclose_ms) {
        SDL_Event quit_event;
        memset(&quit_event, 0, sizeof(quit_event));
        quit_event.type = SDL_QUIT;
        SDL_PushEvent(&quit_event);
        autoclose_ms = -1;
    }
}

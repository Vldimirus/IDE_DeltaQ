// DeltaQ Demo — window1_events.c
#include "window1_events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int counter = 0;
static Uint32 timer_acc = 0;
static Uint32 total_ms = 0;
static int autoclose_ms = -2;

static int read_autoclose_ms(void)
{
    const char *env = getenv("DQ_DESKTOP_UI_FLOW_AUTOCLOSE_MS");
    if (!env || !*env)
        return -1;

    const long value = strtol(env, NULL, 10);
    if (value <= 0 || value > 600000)
        return -1;

    return (int) value;
}

void ui_update(UIState *ui, Uint32 delta_ms) {
    if (autoclose_ms == -2) {
        autoclose_ms = read_autoclose_ms();
        if (autoclose_ms > 0) {
            snprintf(ui->txtLog.text, sizeof(ui->txtLog.text),
                     "Auto-close armed: %d ms\n", autoclose_ms);
        }
    }

    total_ms += delta_ms;
    timer_acc += delta_ms;
    while (timer_acc >= 1000) {
        timer_acc -= 1000;
        counter++;
        snprintf(ui->lblCounter.text, sizeof(ui->lblCounter.text), "%d", counter);
    }

    if (autoclose_ms > 0 && total_ms >= (Uint32) autoclose_ms) {
        SDL_Event quit_event;
        memset(&quit_event, 0, sizeof(quit_event));
        quit_event.type = SDL_QUIT;
        SDL_PushEvent(&quit_event);
        autoclose_ms = -1;
    }
}

void on_btnCopyToLog_click(UIState *ui) {
    char line[128];
    snprintf(line, sizeof(line), "Counter: %d\n", counter);
    size_t remain = sizeof(ui->txtLog.text) - strlen(ui->txtLog.text) - 1;
    strncat(ui->txtLog.text, line, remain);
}

void on_btnInsertText_click(UIState *ui) {
    if (ui->editInput.text[0]) {
        size_t remain = sizeof(ui->txtLog.text) - strlen(ui->txtLog.text) - 1;
        strncat(ui->txtLog.text, ui->editInput.text, remain);
        remain = sizeof(ui->txtLog.text) - strlen(ui->txtLog.text) - 1;
        strncat(ui->txtLog.text, "\n", remain);
        ui->editInput.text[0] = '\0';
    }
}

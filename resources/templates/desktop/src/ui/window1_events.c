// DeltaQ Demo — window1_events.c
#include "window1_events.h"
#include <string.h>
#include <stdio.h>

static int counter = 0;
static Uint32 timer_acc = 0;

void ui_update(UIState *ui, Uint32 delta_ms) {
    timer_acc += delta_ms;
    while (timer_acc >= 1000) {
        timer_acc -= 1000;
        counter++;
        snprintf(ui->lblCounter.text, sizeof(ui->lblCounter.text), "%d", counter);
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

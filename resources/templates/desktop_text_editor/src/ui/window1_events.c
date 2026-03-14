// DeltaQ Template — window1_events.c
#include "window1_events.h"

#include <stdio.h>
#include <string.h>

static void update_status(UIState *ui)
{
    size_t i = 0;
    size_t lines = 0;
    const size_t length = strlen(ui->txtDocument.text);

    if (length > 0)
        lines = 1;

    for (i = 0; i < length; ++i) {
        if (ui->txtDocument.text[i] == '\n')
            ++lines;
    }

    snprintf(ui->lblStatus.text,
             sizeof(ui->lblStatus.text),
             "Lines: %zu | Characters: %zu",
             lines,
             length);
}

void ui_update(UIState *ui, Uint32 delta_ms)
{
    (void)ui;
    (void)delta_ms;
}

void on_btnNewDocument_click(UIState *ui)
{
    strcpy(ui->lblDocName.text, "untitled.txt");
    ui->txtDocument.text[0] = '\0';
    ui->txtDocument.scroll_y = 0;
    ui->editInput.text[0] = '\0';
    update_status(ui);
}

void on_btnInsertSample_click(UIState *ui)
{
    strncpy(ui->txtDocument.text,
            "DeltaQ Text Pad\n"
            "================\n"
            "This starter is distinct from the UI Graph Example.\n"
            "Use Append Line to grow the document.\n"
            "Use New to clear it back to an empty file.\n",
            sizeof(ui->txtDocument.text) - 1U);
    ui->txtDocument.text[sizeof(ui->txtDocument.text) - 1U] = '\0';
    strcpy(ui->lblDocName.text, "notes.txt");
    update_status(ui);
}

void on_btnAppendLine_click(UIState *ui)
{
    if (!ui->editInput.text[0])
        return;

    size_t remain = sizeof(ui->txtDocument.text) - strlen(ui->txtDocument.text) - 1U;
    strncat(ui->txtDocument.text, ui->editInput.text, remain);

    remain = sizeof(ui->txtDocument.text) - strlen(ui->txtDocument.text) - 1U;
    strncat(ui->txtDocument.text, "\n", remain);

    ui->editInput.text[0] = '\0';
    update_status(ui);
}

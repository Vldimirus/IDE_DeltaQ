// DeltaQ Template — window1_events.c
#include "window1_events.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Uint32 total_ms = 0;
static int autoclose_ms = -2;
static int starter_bootstrapped = 0;

static void copy_text(char *dst, size_t size, const char *src)
{
    if (!dst || size == 0U)
        return;

    if (!src)
        src = "";

    strncpy(dst, src, size - 1U);
    dst[size - 1U] = '\0';
}

static int read_autoclose_ms(void)
{
    const char *env = getenv("DQ_DESKTOP_TEXT_EDITOR_AUTOCLOSE_MS");
    if (!env || !*env)
        return -1;

    {
        const long value = strtol(env, NULL, 10);
        if (value <= 0 || value > 600000)
            return -1;
        return (int) value;
    }
}

static size_t count_words(const char *text)
{
    size_t words = 0;
    int in_word = 0;
    size_t i = 0;

    for (i = 0; text[i] != '\0'; ++i) {
        const unsigned char ch = (unsigned char) text[i];
        const int is_space = ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
        if (is_space) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            ++words;
        }
    }

    return words;
}

static void set_session_state(UIState *ui, const char *text)
{
    copy_text(ui->lblSessionState.text, sizeof(ui->lblSessionState.text), text);
}

static void set_status(UIState *ui, const char *text)
{
    copy_text(ui->lblStatus.text, sizeof(ui->lblStatus.text), text);
}

static void update_document_stats(UIState *ui, const char *status_text)
{
    size_t i = 0;
    size_t lines = 0;
    const size_t length = strlen(ui->txtDocument.text);
    const size_t words = count_words(ui->txtDocument.text);

    if (length > 0U)
        lines = 1U;

    for (i = 0; i < length; ++i) {
        if (ui->txtDocument.text[i] == '\n')
            ++lines;
    }

    snprintf(ui->lblDocumentStats.text,
             sizeof(ui->lblDocumentStats.text),
             "Words: %zu | Lines: %zu | Characters: %zu",
             words,
             lines,
             length);
    set_status(ui, status_text);
}

static void apply_document(UIState *ui,
                           const char *doc_name,
                           const char *session_text,
                           const char *status_text,
                           const char *document_text)
{
    copy_text(ui->lblDocName.text, sizeof(ui->lblDocName.text), doc_name);
    copy_text(ui->txtDocument.text, sizeof(ui->txtDocument.text), document_text);
    ui->txtDocument.scroll_y = 0;
    ui->editInput.text[0] = '\0';
    set_session_state(ui, session_text);
    update_document_stats(ui, status_text);
}

static void load_sample_document(UIState *ui)
{
    static const char sample_text[] =
        "Release Readiness Notes\n"
        "======================\n"
        "- Verify the desktop starter build and run path.\n"
        "- Capture one onboarding screenshot for the team.\n"
        "- Replace starter button handlers with project actions.\n"
        "\n"
        "Scratchpad\n"
        "----------\n"
        "Type a note above and click Append Note.\n";

    apply_document(ui,
                   "release_notes.md",
                   "Starter surfaces: menu, toolbar, tabs, status",
                   "Loaded starter sample",
                   sample_text);
}

void ui_update(UIState *ui, Uint32 delta_ms)
{
    if (!starter_bootstrapped) {
        starter_bootstrapped = 1;
        load_sample_document(ui);
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

void on_btnNewDocument_click(UIState *ui)
{
    apply_document(ui,
                   "untitled_notes.md",
                   "Blank draft ready; append notes from scratch",
                   "Created a blank draft",
                   "");
}

void on_btnInsertSample_click(UIState *ui)
{
    load_sample_document(ui);
}

void on_btnAppendLine_click(UIState *ui)
{
    const size_t current_length = strlen(ui->txtDocument.text);
    const size_t input_length = strlen(ui->editInput.text);
    size_t available = 0;

    if (!ui->editInput.text[0]) {
        set_status(ui, "Scratch input is empty");
        return;
    }

    available = sizeof(ui->txtDocument.text) - current_length - 1U;
    if (current_length > 0U && ui->txtDocument.text[current_length - 1U] != '\n') {
        if (available < 2U) {
            set_status(ui, "Document buffer is full");
            return;
        }

        strncat(ui->txtDocument.text, "\n", available);
        available = sizeof(ui->txtDocument.text) - strlen(ui->txtDocument.text) - 1U;
    }

    if (input_length + 1U > available) {
        set_status(ui, "Document buffer is full");
        return;
    }

    strncat(ui->txtDocument.text, ui->editInput.text, available);
    available = sizeof(ui->txtDocument.text) - strlen(ui->txtDocument.text) - 1U;
    strncat(ui->txtDocument.text, "\n", available);
    ui->editInput.text[0] = '\0';
    update_document_stats(ui, "Appended scratch note");
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#endif

#ifndef _WIN32
static struct termios g_original_termios;
static bool g_has_terminal_setup = false;

static void restore_terminal_mode(void)
{
    if (g_has_terminal_setup)
        tcsetattr(STDIN_FILENO, TCSANOW, &g_original_termios);
}

static void setup_terminal_mode(void)
{
    if (!isatty(STDIN_FILENO))
        return;

    if (tcgetattr(STDIN_FILENO, &g_original_termios) != 0)
        return;

    struct termios raw = g_original_termios;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
        g_has_terminal_setup = true;
        atexit(restore_terminal_mode);
    }
}
#endif

static void sleep_ms(unsigned long ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec delay;
    delay.tv_sec = (time_t) (ms / 1000U);
    delay.tv_nsec = (long) ((ms % 1000U) * 1000000UL);
    nanosleep(&delay, NULL);
#endif
}

static bool should_stop(void)
{
#ifdef _WIN32
    if (!_kbhit())
        return false;
    const int ch = _getch();
    return ch == 'q' || ch == 'Q';
#else
    fd_set input_set;
    struct timeval timeout;
    unsigned char ch = 0;

    FD_ZERO(&input_set);
    FD_SET(STDIN_FILENO, &input_set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    if (select(STDIN_FILENO + 1, &input_set, NULL, NULL, &timeout) <= 0)
        return false;

    if (read(STDIN_FILENO, &ch, 1) != 1)
        return false;

    return ch == 'q' || ch == 'Q';
#endif
}

int main(void)
{
#ifndef _WIN32
    setup_terminal_mode();
#endif

    puts("Press q to stop the counter.");

    for (int counter = 0;; ++counter) {
        printf("%d\n", counter);
        fflush(stdout);

        if (should_stop())
            break;

        sleep_ms(500);
    }

    puts("Counter stopped.");
    return 0;
}

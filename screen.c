#include <stdlib.h>

#include "screen.h"

ncurwin *init_screen()
{
    ncurwin *winconf = malloc(sizeof(ncurwin));
    winconf->win = initscr();
    winconf->lines = LINES;
    winconf->cols = COLS;
    raw();
    nonl();
    noecho();
    keypad(winconf->win, true); // Enable handling of function keys and arrow keys.
    set_escdelay(50);           // Pass ESC faster.

    return winconf;
}

bool is_cursor_at_top_left_end(int x, int y)
{
    return x == 0 && y == 0;
}

bool is_cursor_at_bottom_right_end(ncurwin *winconf, int x, int y)
{
    return x == winconf->cols - 1 && y == winconf->lines - 1;
}

void add_standout_char(WINDOW *win, const char c)
{
    wattron(win, A_STANDOUT);
    waddch(win, c);
    wattroff(win, A_STANDOUT);
}

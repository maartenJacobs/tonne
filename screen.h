#include <ncursesw/curses.h>

typedef struct ncurses_window
{
    WINDOW *win;
    int lines;
    int cols;
} ncurwin;

bool is_cursor_at_top_left_end(int x, int y);
bool is_cursor_at_bottom_right_end(ncurwin *winconf, int x, int y);
ncurwin *init_screen();
void add_standout_char(WINDOW *win, const char c);

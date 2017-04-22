#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fslice.h"

typedef struct ncurses_window
{
    WINDOW *win;
    int lines;
    int cols;
} ncurwin;

typedef struct editor
{
    ncurwin *winconf;
    fslice *slice; // Lines of the file that are currently visible.
} editor;

void fatal(const char *message)
{
    puts("UNABLE TO CONTINUE");
    puts(message);
    exit(255);
}

void free_editor(editor *state)
{
    // Close the ncurses window.
    endwin();
    free(state->winconf);

    // Free the slice of the file.
    free_slice(state->slice);
    free(state->slice);

    // Free the remaining memory.
    free(state);
}

// TODO: long lines have been truncated. Display horizontal ellipsis and allow slice movement to
// display the rest of the line.
void update_and_print_file_slice(editor *state, unsigned int start, const unsigned int no_of_lines)
{
    update_slice(state->slice, start, no_of_lines);

    // Print the slice.
    wclear(state->winconf->win);
    fline *curr = state->slice->line_data;
    while (curr != NULL)
    {
        waddnstr(state->winconf->win, curr->data,
                 curr->len > state->winconf->cols ? state->winconf->cols - 1 : curr->len);
        if (curr->len > state->winconf->cols)
        {
            waddch(state->winconf->win, '\n');
        }
        curr = curr->next;
    }
    wrefresh(state->winconf->win);
}

void init_window(editor *state)
{
    // Start displaying the UI.
    state->winconf = malloc(sizeof(ncurwin));
    state->winconf->win = initscr();
    state->winconf->lines = LINES;
    state->winconf->cols = COLS;
    cbreak();                          // Do not wait for ENTER to send characters.
    raw();                             // Pass on any entered characters.
    noecho();                          // Do not print entered characters.
    keypad(state->winconf->win, true); // Enable handling of function keys and arrow keys.

    // Print the first n lines of the file to the screen.
    update_and_print_file_slice(state, 0, state->winconf->lines);

    // Move the cursor to the start position.
    wmove(state->winconf->win, 0, 0);
}

void move_slice(editor *state, int offset_in_lines)
{
    update_and_print_file_slice(state, state->slice->start_line + offset_in_lines, state->winconf->lines);
}

bool is_cursor_at_bottom_right_end(editor *state, int x, int y)
{
    return x == state->winconf->cols - 1 && y == state->winconf->lines - 1;
}

bool is_cursor_at_top_left_end(int x, int y)
{
    return x == 0 && y == 0;
}

int main(int argc, char *argv[])
{
    // Check if a filename was passed.
    if (argc != 2)
    {
        puts("Usage: tonne <file>");
        return 0;
    }

    // Build the editor state.
    editor *state = malloc(sizeof(editor));
    state->slice = new_slice();

    // Open the file for reading.
    state->slice->fd = fopen(argv[1], "r");
    if (state->slice->fd == NULL)
    {
        puts("Unable to open file");
        free(state->slice);
        free(state);
        return 1;
    }

    // Initialise the window with the first slice of the file.
    init_window(state);

    // Process characters as commands.
    int comm, x, y;
    bool keep_processing = true;
    do
    {
        getyx(state->winconf->win, y, x);

        comm = getch();
        switch (comm)
        {
        case 27:
            keep_processing = false;
            break;
        case KEY_LEFT:
            if (is_cursor_at_top_left_end(x, y) && state->slice->start_line != 0)
            {
                move_slice(state, -1);

                // Go to the previous line at the end.
                wmove(state->winconf->win, 0, state->winconf->cols - 1);
            }
            else if (is_cursor_at_top_left_end(x, y))
            {
                // We are at the top left corner and there are no lines before this line.
            }
            else if (x == 0)
            {
                // Go to the previous line at the end.
                wmove(state->winconf->win, y - 1, state->winconf->cols - 1);
            }
            else
            {
                wmove(state->winconf->win, y, x - 1);
            }
            break;
        case KEY_RIGHT:
            if (is_cursor_at_bottom_right_end(state, x, y) && has_next_line(state->slice))
            {
                move_slice(state, 1);
            }
            else if (is_cursor_at_bottom_right_end(state, x, y))
            {
                // There is no next line and we are at the end of the line.
            }
            else if (x == state->winconf->cols - 1)
            {
                // Go to the next line at the start.
                wmove(state->winconf->win, y + 1, 0);
            }
            else
            {
                wmove(state->winconf->win, y, x + 1);
            }
            break;
        case KEY_UP:
            if (y == 0 && has_previous_line(state->slice))
            {
                move_slice(state, -1);
                wmove(state->winconf->win, 0, x);
            }
            else if (y == 0)
            {
                // There are no previous lines and we are at the start of the screen.
            }
            else
            {
                wmove(state->winconf->win, y - 1, x);
            }
            break;
        case KEY_DOWN:
            if (y == state->winconf->lines - 1 && has_next_line(state->slice))
            {
                // TODO: x will now be the length of the new line, rather than the previous x
                move_slice(state, 1);
                wmove(state->winconf->win, y, x);
            }
            else if (y == state->winconf->lines - 1)
            {
                // We are at the bottom line and there are no lines after this line.
            }
            else
            {
                wmove(state->winconf->win, y + 1, x);
            }
            break;
        }
    } while (keep_processing);

    // Free used resources.
    free_editor(state);
    puts("Bye!");
    return 0;
}
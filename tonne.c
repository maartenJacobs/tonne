#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ncurses_window
{
    WINDOW *win;
    int lines;
    int cols;
} ncurwin;

typedef struct editor
{
    FILE *fd;
    ncurwin *winconf;
    char *slice; // Lines of the file that are currently visible.
    int slice_start;
    int no_of_lines;
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

    // Close the opened file.
    fclose(state->fd);

    // Free the remaining memory.
    free(state->slice);
    free(state);
}

void update_and_print_file_slice(editor *state, unsigned int start, const unsigned int no_of_lines)
{
    wclear(state->winconf->win);

    // TODO: better slice handling. No point recalculating the entire slice.
    if (state->slice != NULL)
    {
        free(state->slice);
    }
    state->slice = malloc(sizeof(char) * 4096 * state->winconf->lines);
    state->slice[0] = '\0';
    state->slice_start = start;
    state->no_of_lines = no_of_lines;

    // TODO: limit line size to <columns> characters.
    // TODO: deal with lines longer than <columns> characters.
    size_t line_size = sizeof(char) * 4096;
    char *line = malloc(line_size);
    ssize_t line_result;

    // First skip the lines before the start.
    fseek(state->fd, 0, SEEK_SET);
    while (start != 0)
    {
        line_result = getline(&line, &line_size, state->fd);
        start--;

        if (line_result == EOF)
        {
            return;
        }
        // TODO: handle file error
    }
    // Handle a file that has fewer lines than the offset.

    // Get the number of lines from the offset.
    unsigned int i = 0, slice_offset = 0;
    size_t line_len;
    do
    {
        line_result = getline(&line, &line_size, state->fd);
        line_len = strlen(line);

        // Store the line.
        memcpy(state->slice + slice_offset, line, line_len);
        slice_offset += line_len;
    } while (line_result != EOF && line_result != -1 && ++i < no_of_lines);
    state->slice[slice_offset] = '\0';

    // Free the used resources.
    free(line);

    // Print the slice.
    waddstr(state->winconf->win, state->slice);
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
    state->slice = NULL;
    update_and_print_file_slice(state, 0, state->winconf->lines);

    // Move the cursor to the start position.
    wmove(state->winconf->win, 0, 0);
}

// TODO: BUG - does not recognise next line past line count - 2
bool has_next_line(editor *state)
{
    size_t line_size = sizeof(char) * 4096;
    char *line = malloc(line_size);
    if (line == NULL)
    {
        fatal("Unable to allocate memory for line");
    }
    line[0] = '\0';

    ssize_t line_result = getline(&line, &line_size, state->fd);
    size_t line_len = strlen(line);
    free(line);

    if (line_result != -1)
    {
        // TODO: handle file error.
    }
    if (line_result == EOF)
    {
        return false;
    }

    // Undo line read.
    fseek(state->fd, -line_len, SEEK_CUR);

    // Report that there is a next line.
    return true;
}

bool has_previous_line(editor *state)
{
    // Store the original position in the file.
    fpos_t *pos = malloc(sizeof(fpos_t));
    fgetpos(state->fd, pos);

    // Move the file descriptor to the beginning of the slice
    // minus 1 character.
    fseek(state->fd, -strlen(state->slice) - 1, SEEK_CUR);
    int prev = fgetc(state->fd);

    // Restore the original position.
    fsetpos(state->fd, pos);
    free(pos);

    return prev == '\n';
}

void move_slice(editor *state, int offset_in_lines)
{
    update_and_print_file_slice(state, state->slice_start + offset_in_lines, state->winconf->lines);
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

    // Open the file for reading.
    state->fd = fopen(argv[1], "r");
    if (state->fd == NULL)
    {
        puts("Unable to open file");
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
            if (is_cursor_at_top_left_end(x, y) && state->slice_start != 0)
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
            if (is_cursor_at_bottom_right_end(state, x, y) && has_next_line(state))
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
            if (y == 0 && has_previous_line(state))
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
            if (y == state->winconf->lines - 1 && has_next_line(state))
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
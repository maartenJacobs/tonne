#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ncurses_window
{
    WINDOW *win;
    int lines;
} ncurwin;

typedef struct editor
{
    FILE *fd;
    ncurwin *winconf;
    char *slice; // Lines of the file that are currently visible.
    int slice_start;
    int no_of_lines;
} editor;

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
    while (start != 0 && line_result != EOF && line_result != -1)
    {
        line_result = getline(&line, &line_size, state->fd);
        start--;
    }
    // Handle a file that has fewer lines than the offset.
    if (line_result == EOF)
    {
        return;
    }
    // TODO: handle file error

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
    cbreak();                          // Do not wait for ENTER to send characters.
    raw();                             // Pass on any entered characters.
    noecho();                          // Do not print entered characters.
    keypad(state->winconf->win, true); // Enable handling of function keys and arrow keys.

    // Print the first n lines of the file to the screen.
    update_and_print_file_slice(state, 0, state->winconf->lines);

    // Move the cursor to the start position.
    wmove(state->winconf->win, 0, 0);
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
            if (x == 0)
            {
                if (y == 0)
                {
                    // TODO: check if there is a line above this one in the file.
                    break;
                }

                // Go to the previous line at the end.
                wmove(state->winconf->win, y - 1, COLS - 1);
            }
            else
            {
                wmove(state->winconf->win, y, x - 1);
            }
            break;
        case KEY_RIGHT:
            if (x == COLS - 1)
            {
                if (y == LINES - 1)
                {
                    // TODO: check if there is a line above this one in the file.
                    break;
                }

                // Go to the next line at the start.
                wmove(state->winconf->win, y + 1, 0);
            }
            else
            {
                wmove(state->winconf->win, y, x + 1);
            }
            break;
        case KEY_UP:
            if (y == 0)
            {
                // TODO: check if there is a line above this one in the file.
                break;
            }
            wmove(state->winconf->win, y - 1, x);
            break;
        case KEY_DOWN:
            if (y == LINES)
            {
                // TODO: check if there is a line below this one in the file.
                break;
            }
            wmove(state->winconf->win, y + 1, x);
            break;
        }
    } while (keep_processing);

    // Free used resources.
    free_editor(state);
    puts("Bye!");
    return 0;
}
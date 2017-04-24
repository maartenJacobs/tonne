#include <stdlib.h>
#include <string.h>

#include "editor.h"

editor *new_editor(char *edit_file)
{
    editor *state = malloc(sizeof(editor));
    state->filename = malloc(sizeof(char) * strlen(edit_file));
    strcpy(state->filename, edit_file);
    state->slice = new_slice();

    // Open the file for reading.
    state->slice->fd = fopen(state->filename, "r");
    if (state->slice->fd == NULL)
    {
        free(state->slice);
        free(state);
        return NULL;
    }

    // Start displaying the UI.
    state->winconf = init_screen();

    // Print the first n lines of the file to the screen.
    update_and_print_file_slice(state, 0, state->winconf->lines);

    // Move the cursor to the start position.
    wmove(state->winconf->win, 0, 0);

    return state;
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
    free(state->filename);
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

long int foffset_from_pos(editor *state, int y, int x)
{
    long int add_offset = state->slice->start_offset + x;
    int row_offset = y;
    fline *line = state->slice->line_data;
    while (line != NULL && row_offset-- > 0)
    {
        add_offset += line->len;
        line = line->next;
    }
    return add_offset;
}

void move_slice(editor *state, int offset_in_lines)
{
    update_and_print_file_slice(state, state->slice->start_line + offset_in_lines, state->winconf->lines);
}

void process_commands(editor *state)
{
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
            if (is_cursor_at_bottom_right_end(state->winconf, x, y) && has_next_line(state->slice))
            {
                move_slice(state, 1);
            }
            else if (is_cursor_at_bottom_right_end(state->winconf, x, y))
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
        case KEY_DC:
        case KEY_BACKSPACE:
            // Delete the character just before the cursor.
            {
                long int del_offset = foffset_from_pos(state, y, x) - (comm == KEY_DC ? 0 : 1);
                if (del_offset < 0)
                {
                    break;
                }
                alter_file(state->slice, state->filename, OP_DELETE, del_offset, 0);

                // Update the displayed file.
                update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);
                wmove(state->winconf->win, y, x - (comm == KEY_DC ? 0 : 1));
            }
            break;
        default:
            if (comm > 0 && comm < 127) // Any other ASCII value
            {
                // Add in the character.
                alter_file(state->slice, state->filename, OP_ADD, foffset_from_pos(state, y, x), comm);

                // Update the displayed file.
                update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);
                wmove(state->winconf->win, y, x + 1);
            }
            break;
        }
    } while (keep_processing);
}
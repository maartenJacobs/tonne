#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "editor.h"

// TODO: this is an experiment with using assert. Is it good or bad practice?
// Wrap asserts in `#ifdef DEBUG` block?
static void move_cursor(editor *state, unsigned int new_y, unsigned int new_x)
{
    // Assert correct usage of cursor movement: the editing window is limited to
    // column MIN_COLS to column MAX_COLS(state), ensuring the control columns are
    // not editable.
    // TODO: use multiple windows instead? Can ncurses windows be vertical and stacked
    // horizontally?
    assert(new_x >= MIN_COLS);
    assert(new_x <= MAX_COLS(state));
    assert(new_y >= 0);
    assert(new_y <= MAX_LINES(state));

    wmove(state->winconf->win, new_y, new_x);
}

editor *new_editor(char *edit_file)
{
    editor *state = malloc(sizeof(editor));
    state->filename = malloc(sizeof(char) * strlen(edit_file));
    strcpy(state->filename, edit_file);
    state->slice = new_slice();
    state->left_margin = 0;

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
    // As we need at least 1 column to display the ellipsis,
    // the ncurses view needs to provide at least 1 column.
    assert(state->winconf->cols > 1);

    // Print the first n lines of the file to the screen.
    update_and_print_file_slice(state, 0, state->winconf->lines);

    // Move the cursor to the start position.
    move_cursor(state, 0, MIN_COLS);

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

static unsigned int uimin(unsigned int a, unsigned int b)
{
    if (a == b)
    {
        return a;
    }
    return (a > b ? b : a);
}

void print_line(editor *state, fline *line)
{
    // Print the left control column.
    if (state->left_margin)
    {
        add_standout_char(state->winconf->win, '<');
    }
    else
    {
        waddch(state->winconf->win, ' ');
    }

    // Determine if we have to display the line at all.
    if (state->left_margin >= line->len)
    {
        waddch(state->winconf->win, '\n');
        return;
    }

    // Print the portion of the line that fits within the ncurses window.
    size_t len = line->len - state->left_margin;
    waddnstr(state->winconf->win, line->data + state->left_margin,
             uimin(len, MAX_COLS(state)));

    // Print the right control column.
    if (len > MAX_COLS(state))
    {
        add_standout_char(state->winconf->win, '>');
    }
}

// TODO: long lines have been truncated. Display horizontal ellipsis and allow slice movement to
// display the rest of the line.
void update_and_print_file_slice(editor *state, unsigned int line_offset, const unsigned int no_of_lines)
{
    update_slice(state->slice, line_offset, no_of_lines);

    // Print the slice.
    wclear(state->winconf->win);
    state->displayed_lines = 0;
    fline *curr = state->slice->line_data;
    while (curr != NULL)
    {
        print_line(state, curr);
        state->displayed_lines++;
        curr = curr->next;
    }
    wrefresh(state->winconf->win);
}

long int foffset_from_pos(editor *state, int y, int x)
{
    long int offset = state->slice->start_offset + state->left_margin + x - MIN_COLS;
    int row_offset = y;
    fline *line = state->slice->line_data;
    while (line != NULL && row_offset-- > 0)
    {
        offset += line->len;
        line = line->next;
    }
    return offset;
}

void move_slice(editor *state, int offset_in_lines)
{
    update_and_print_file_slice(state, state->slice->start_line + offset_in_lines, state->winconf->lines);
}

bool is_line_longer_than(fline *line, unsigned int col_offset)
{
    return line->len > col_offset;
}

fline *line_offset_to_fline(fslice *slice, unsigned int line_offset)
{
    fline *line = slice->line_data;
    while (line_offset-- != 0)
        line = line->next;
    return line;
}

static bool at_end_of_line(editor *state, fline *line, int x)
{
    return (state->left_margin + x - MIN_COLS + 1) == line->len;
}

static bool at_last_cursor_position(editor *state, int y, int x)
{
    return y == MAX_LINES(state) &&
           at_end_of_line(state, line_offset_to_fline(state->slice, MAX_LINES(state)), x);
}

static bool at_end_of_screen(editor *state, int x)
{
    return x == MAX_COLS(state);
}

void process_commands(editor *state)
{
    int comm, x, y;
    bool keep_processing = true;
    fline *line;
    do
    {
        getyx(state->winconf->win, y, x);
        line = line_offset_to_fline(state->slice, y);

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
                move_cursor(state, 1, MAX_COLS(state));
            }
            else if (is_cursor_at_top_left_end(x, y))
            {
                // We are at the top left corner and there are no lines before this line.
            }
            else if (x == MIN_COLS)
            {
                if (state->left_margin > 0)
                {
                    state->left_margin--;
                    update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);

                    // Reset the cursor position.
                    move_cursor(state, y, x);
                }
                else
                {
                    // Go to the previous line at the end.
                    move_cursor(state, y - 1, MAX_COLS(state));
                }
            }
            else
            {
                move_cursor(state, y, x - 1);
            }
            break;
        case KEY_RIGHT:
            if (at_last_cursor_position(state, y, x))
            {
                if (has_next_line(state->slice))
                {
                    move_slice(state, 1);
                }
            }
            else if (at_end_of_screen(state, x) || at_end_of_line(state, line, x))
            {
                if (at_end_of_line(state, line, x))
                {
                    if (state->left_margin)
                    {
                        // Reset the left margin.
                        state->left_margin = 0;

                        update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);
                    }

                    // Go to the next line at the start.
                    move_cursor(state, y + 1, MIN_COLS);
                }
                else
                {
                    state->left_margin++;
                    update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);

                    // Reset the cursor position.
                    move_cursor(state, y, x);
                }
            }
            else
            {
                move_cursor(state, y, x + 1);
            }
            break;
        case KEY_UP:
            if (y == 0 && has_previous_line(state->slice))
            {
                move_slice(state, -1);
                move_cursor(state, 0, x);
            }
            else if (y == 0)
            {
                // There are no previous lines and we are at the start of the screen.
            }
            else
            {
                move_cursor(state, y - 1, x);
            }
            break;
        case KEY_DOWN:
            if (y == MAX_LINES(state) && has_next_line(state->slice))
            {
                // TODO: x will now be the length of the new line, rather than the previous x
                move_slice(state, 1);
                move_cursor(state, y, x);
            }
            else if (y == MAX_LINES(state))
            {
                // We are at the bottom line and there are no lines after this line.
            }
            else
            {
                move_cursor(state, y + 1, x);
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
                move_cursor(state, y, x - (comm == KEY_DC ? 0 : 1));
            }
            break;
        default:
            if (comm > 0 && comm < 127) // Any other ASCII value
            {
                // Add in the character.
                alter_file(state->slice, state->filename, OP_ADD, foffset_from_pos(state, y, x), comm);

                // Update the displayed file.
                if (x == MAX_COLS(state))
                {
                    state->left_margin++;
                    x--;
                }
                update_and_print_file_slice(state, state->slice->start_line, state->winconf->lines);
                move_cursor(state, y, x + 1);
            }
            break;
        }
    } while (keep_processing);
}
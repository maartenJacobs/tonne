#include "screen.h"
#include "fslice.h"

// Only columns 1 to COLS - 1 are used to display characters
// of the file slice. The remaining columns are used to indicate
// currently non-displayed characters.
#define MIN_COLS (1)
#define MAX_COLS(state) (state->winconf->cols - 2)
#define MAX_LINES(state) (state->displayed_lines - 1)

typedef struct editor
{
    char *filename;
    ncurwin *winconf;
    fslice *slice;                // Lines of the file that are currently visible.
    unsigned int left_margin;     // The number of columns on the left that are not displayed.
    unsigned int displayed_lines; // The number of lines currently displayed.
} editor;

editor *new_editor();
void free_editor(editor *state);
void update_and_print_file_slice(editor *state, unsigned int start, const unsigned int no_of_lines);
long int foffset_from_pos(editor *state, int y, int x);
void move_slice(editor *state, int offset_in_lines);
void process_commands(editor *state);

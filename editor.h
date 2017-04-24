#include "screen.h"
#include "fslice.h"

typedef struct editor
{
    char *filename;
    ncurwin *winconf;
    fslice *slice; // Lines of the file that are currently visible.
} editor;

editor *new_editor();
void free_editor(editor *state);
void update_and_print_file_slice(editor *state, unsigned int start, const unsigned int no_of_lines);
long int foffset_from_pos(editor *state, int y, int x);
void move_slice(editor *state, int offset_in_lines);
void process_commands(editor *state);

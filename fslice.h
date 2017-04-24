#include <stdio.h>

// File operations.
#define OP_ADD 1
#define OP_DELETE 2

typedef struct file_line
{
    size_t len;
    char *data;
    struct file_line *next;
} fline;

typedef struct file_slice
{
    FILE *fd;              // File of which we have a slice.
    fline *line_data;      // Linked list of lines of the file that can be displayed.
    int start_line;        // Line where the slice starts in the file.
    long int start_offset; // Offset in bytes where the slice starts in the file.
    int no_of_lines;       // Number of lines of the slice.
} fslice;

fslice *new_slice();
void free_slice(fslice *slice);
void update_slice(fslice *slice, unsigned int start, unsigned int no_of_lines);
bool has_next_line(fslice *slice);
bool has_previous_line(fslice *slice);
int alter_file(fslice *slice, char *fname, const short op, unsigned long int op_offset, char arg);

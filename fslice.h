#include <stdio.h>

typedef struct file_slice
{
    FILE *fd;        // File of which we have a slice.
    char *line_data; // Lines of the file that are currently visible.
    int start_line;  // Line where the slice starts in the file.
    int no_of_lines; // Number of lines of the slice.
} fslice;

void free_slice(fslice *slice);
void update_slice(fslice *slice, unsigned int start, const unsigned int no_of_lines);
bool has_next_line(fslice *slice);
bool has_previous_line(fslice *slice);
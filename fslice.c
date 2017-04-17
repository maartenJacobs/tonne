#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fslice.h"

void update_slice(fslice *slice, unsigned int start, const unsigned int no_of_lines)
{
    // TODO: better slice handling. No point recalculating the entire slice.
    if (slice->line_data != NULL)
    {
        free(slice->line_data);
    }
    slice->start_line = start;
    slice->no_of_lines = no_of_lines;
    slice->line_data = malloc(sizeof(char) * 4096 * slice->no_of_lines);
    slice->line_data[0] = '\0';

    // TODO: limit line size to <columns> characters.
    // TODO: deal with lines longer than <columns> characters.
    size_t line_size = sizeof(char) * 4096;
    char *line = malloc(line_size);
    ssize_t line_result;

    // First skip the lines before the start.
    fseek(slice->fd, 0, SEEK_SET);
    while (start != 0)
    {
        line_result = getline(&line, &line_size, slice->fd);
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
        line_result = getline(&line, &line_size, slice->fd);
        line_len = strlen(line);

        // Store the line.
        memcpy(slice->line_data + slice_offset, line, line_len);
        slice_offset += line_len;
    } while (line_result != EOF && line_result != -1 && ++i < slice->no_of_lines);
    slice->line_data[slice_offset] = '\0';

    // Free the used resources.
    free(line);
}

void free_slice(fslice *slice)
{
    fclose(slice->fd);
    free(slice->line_data);
}

// TODO: BUG - does not recognise next line past line count - 2
bool has_next_line(fslice *slice)
{
    size_t line_size = sizeof(char) * 4096;
    char *line = malloc(line_size);
    if (line == NULL)
    {
        fatal("Unable to allocate memory for line");
    }
    line[0] = '\0';

    ssize_t line_result = getline(&line, &line_size, slice->fd);
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
    fseek(slice->fd, -line_len, SEEK_CUR);

    // Report that there is a next line.
    return true;
}

bool has_previous_line(fslice *slice)
{
    // Store the original position in the file.
    fpos_t *pos = malloc(sizeof(fpos_t));
    fgetpos(slice->fd, pos);

    // Move the file descriptor to the beginning of the slice
    // minus 1 character.
    fseek(slice->fd, -strlen(slice->line_data) - 1, SEEK_CUR);
    int prev = fgetc(slice->fd);

    // Restore the original position.
    fsetpos(slice->fd, pos);
    free(pos);

    return prev == '\n';
}

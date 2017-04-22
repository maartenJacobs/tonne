#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fslice.h"

void free_lines(fslice *slice)
{
    fline *prev = NULL,
          *curr = slice->line_data;
    while (curr != NULL)
    {
        free(curr->data);
        prev = curr;
        curr = curr->next;
        free(prev);
    }
}

size_t slice_len(fslice *slice)
{
    fline *curr = slice->line_data;
    size_t len = 0;
    while (curr != NULL)
    {
        len += curr->len;
        curr = curr->next;
    }
    return len;
}

void update_slice(fslice *slice, unsigned int start, unsigned int no_of_lines)
{
    // TODO: better slice handling. No point recalculating the entire slice.
    if (slice->line_data != NULL)
    {
        free_lines(slice);
    }
    slice->start_line = start;
    slice->no_of_lines = no_of_lines;

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

    // Get the lines from the offset.
    size_t line_len = 0;
    fline *curr = NULL, *prev = NULL;
    while (no_of_lines-- > 0)
    {
        line_result = getline(&line, &line_size, slice->fd);
        line_len = strlen(line);

        if (line_result == EOF || line_result == -1)
        {
            break;
        }

        // Store the line.
        curr = malloc(sizeof(fline));
        curr->next = NULL;
        curr->len = line_len;
        curr->data = malloc(sizeof(char) * curr->len);
        memcpy(curr->data, line, curr->len);
        if (prev == NULL)
        {
            slice->line_data = curr;
        }
        else
        {
            prev->next = curr;
        }
        prev = curr;
    }

    // Free the used resources.
    free(line);
}

void free_slice(fslice *slice)
{
    fclose(slice->fd);
    free_lines(slice);
}

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
    fseek(slice->fd, -slice_len(slice) - 1, SEEK_CUR);
    int prev = fgetc(slice->fd);

    // Restore the original position.
    fsetpos(slice->fd, pos);
    free(pos);

    return prev == '\n';
}

fslice *new_slice()
{
    fslice *slice = malloc(sizeof(fslice));
    slice->line_data = NULL;
    slice->fd = NULL;
    slice->no_of_lines = 0;
    slice->start_line = 0;
    return slice;
}

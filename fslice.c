#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "fslice.h"

#define BUFFER_SIZE 255

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

    size_t line_size = sizeof(char) * BUFFER_SIZE;
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

    // Store the start position of the slice.
    slice->start_offset = ftell(slice->fd);

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
    size_t line_size = sizeof(char) * 2;
    char *line = malloc(line_size);
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

int alter_file(fslice *slice, char *fname, const short op, unsigned long int op_offset, char arg)
{
    // Make a temporary file in the same directory. The temporary
    // file name will be the same as the edited file, but with a tilde
    // at the start.
    char *wfname = malloc(sizeof(char) * strlen(fname) + 2);
    wfname[0] = '~';
    strcpy(wfname + 1, fname);
    FILE *wfd = fopen(wfname, "w");

    // Write the characters before the position we want to manipulate.
    char *buf = malloc(sizeof(char) * BUFFER_SIZE);
    fseek(slice->fd, 0, SEEK_SET);
    unsigned long int read_n = 0;
    for (long int offset = 0; offset < op_offset; offset += read_n)
    {
        read_n = (op_offset - offset + 1) > BUFFER_SIZE ? BUFFER_SIZE : (op_offset - offset + 1);
        if (fgets(buf, read_n, slice->fd) == NULL)
        {
            return -1; // Unable to read data before offset.
        }
        read_n = strlen(buf);
        fputs(buf, wfd);
    }

    // Manipulate the character at this position.
    if (op == OP_ADD)
    {
        // Write the character we want to insert.
        fputc(arg, wfd);
    }
    else if (op == OP_DELETE)
    {
        // Skip the character we want to delete.
        fseek(slice->fd, 1, SEEK_CUR);
    }

    // Write the characters after the inserted character.
    size_t line_size = sizeof(char) * BUFFER_SIZE;
    while (getline(&buf, &line_size, slice->fd) != -1)
    {
        fputs(buf, wfd);
    }
    free(buf);
    fflush(wfd);

    // Overwrite the file and reopen the file we're editing.
    rename(wfname, fname);
    fclose(wfd);
    free(wfname);
    slice->fd = freopen(fname, "r", slice->fd);

    return 0;
}

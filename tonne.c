#include <ncursesw/curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <locale.h>

#include "editor.h"

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    // Check if a filename was passed.
    if (argc != 2)
    {
        puts("Usage: tonne <file>");
        return 0;
    }

    // Build the editor state.
    // Initialises the window with the first slice of the file.
    editor *state = new_editor(argv[1]);
    if (state == NULL)
    {
        printf("Unable to edit file %s\n", argv[1]);
        return 1;
    }

    // Process characters as commands.
    process_commands(state);

    // Free used resources.
    free_editor(state);
    puts("Bye!");
    return 0;
}
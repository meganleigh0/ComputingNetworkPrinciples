#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "InputManage.h"


void emptyBuffer()
{
     // Function to empty the character string buffer
     
    int c = 0;
    while (c != '\n' && c != EOF) {

        c = getchar();
    }
}


int reading(char *chain, size_t length)
{
    // Function that adds what the user has to type in the variable passed as a parameter


    char *positionBetween = NULL;

    // Bind keyboard input
    if (fgets(chain, (int) length, stdin) != NULL) {

        positionBetween = strchr(chain, '\n'); // Function that returns the pointer to the first occurrence found

        if (positionBetween != NULL) {

            *positionBetween = '\0'; // On change the \n at the end of the string
        } else {

            emptyBuffer();
            // If the string is too long,  empty the buffer
        }
        return 1; // 1 for success

    } else {

        return 0; // 0 for error
    }
}


int deserialize_game(char *buffer, Games game)
{
    int y, x;
    for (y = 0; y < NLINE; y++) {
        for (x = 0; x < NCOL; x++) {
            game[x][y] = buffer[y * NCOL + x] - '0';
        }
    }
}

char *substring(const char *s, unsigned int start, unsigned int end)
// Get a portion of a String from start to end
{
    char *new_s = NULL;

    if (s != NULL && start < end)
    {

        new_s = malloc (sizeof (*new_s) * (end - start + 2));
        if (new_s != NULL)
        {
            int i;


            for (i = start; i <= end; i++)
            {

                new_s[i-start] = s[i];
            }
            new_s[i-start] = '\0';
        }
        else
        {
            fprintf (stderr, "Memoire insuffisante\n");
            exit (EXIT_FAILURE);
        }
    }
    return new_s;
}

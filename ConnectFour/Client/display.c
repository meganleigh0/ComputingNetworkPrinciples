#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "InputManage.h"



void pinUp(Games game)
{
// Graphical interface for the client
    const char letterplayer[4] = {'.', 'X', 'O','\0'};

printf("Printing the game board");
printf("\n");
    // Top edge of the board
    printf("+");
    for (int i = 0; i < NCOL; i++) {
        printf("++");
    }
    printf("\n");

    // Loop to move up inside
    for (int i = 0; i < NLINE; i++) { // Go through the lines (y)

        printf("|"); 
        for (int j = 0; j < NCOL; j++) { // Go through the lines (x)
            printf("%c|", letterplayer[game[j][i]]);
        }
        printf("\n");

        // separating line between 2 lines of players
        printf("+");
        for (int a = 0; a < NCOL; a++) {
            printf("++");
        }
        printf("\n");

    }

}

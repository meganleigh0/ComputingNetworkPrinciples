#include <printf.h>
#include "GameLogic.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>




// Constants 
int fullColumn(Games game, int col)
{
    //Test if the box is not empty (return true) or full (return false)

    // if the last box in the column is empty, return = false (not full)
    // else return true (necessarily full)

    if (game[col][0] == 0) {
        return 0;
    } else {
        return 1;
    }
}


int collumnFull(Games game)
{

     //Check if the tray is full and return true if full

    // For loop: if one of the columns is not full,  immediately return false.
    // Otherwise check each column, if all full,  return true.

    for (int x = 0; x < NCOL; x++) {
        if (!fullColumn(game, x)) {
            return 0;
        }
    }
    return 1;
}


int initgame(Games game)
{

    // Write "empty" in all the squares of the board
    for (int y = 0; y < NLINE; y++) {
        for (int x = 0; x < NCOL; x++) {
            game[x][y] = 0;
        }
    }
    return 1;
}


int playPawn(Games game, int col, int player)
{

    // Add a player player to the board at the indicated column

    // returns the row where the player was placed

    for (int y = NLINE - 1; y >= 0; y--) {
        if (game[col][y] == 0) {
            game[col][y] = player + 1;
            return y;
        }
    }
    // If no player could be placed, throw error
    return -1;
}


int verifyPawn(Games game, coord sens, coord position, int player)
{

    // Test if 4 plays of the current player are aligned.
    // The search direction depends on direction
    // Following the direction, the check position advances to the last player of the current player.
    // Then the direction is reversed, the position is incremented and the countplayplayer counter is incremented if the player belongs to the current player.
    // If countplayplayer counter reaches 4, return TRUE. Otherwise returns FALSE.

    int countPawnPlays = 0; // Count of successive pawns/moves

    // Set the coordinates of the position to be checked to coordinate, shift one iteration.
    position.x += sens.x;
    position.y += sens.y;

    // Loop to advance to the end of the succession of plays of the current player
    while (game[position.x][position.y] == (player + 1) &&
           ((position.x >= 0 && position.x < NCOL) && (position.y >= 0 && position.y < NLINE))) {
        position.x += sens.x;
        position.y += sens.y;
    }

     // Reverse search direction
    sens.x = -sens.x;
    sens.y = -sens.y;

    position.x += sens.x;
    position.y += sens.y;

     // loop to count the number of successive plays of the current player   
    while (game[position.x][position.y] == (player + 1) &&
           ((position.x >= 0 && position.x < NCOL) && (position.y >= 0 && position.y < NLINE))) {
        countPawnPlays += 1;
        position.x += sens.x;
        position.y += sens.y;

    }

    // if the counter reaches 4, return TRUE
    if (countPawnPlays >= 4) {
        return 1;
    }
    // There are not 4 successive plays of the current player, returns FALSE
    return 0;
}


int pawnWins(Games game, coord coordinate, int player)
{

    //Tests if the pawn wins.
    // Returns TRUE if the pawn wins the game for the current player
    // Returns FALSE otherwise

    //search[]: allows the incrementation of the position to check the pawns
    // Vertical - Horizontal - BottomRight Diagonal - TopRight Diagonalle pawns
    // Vertical - Horizontal - Diagonale BasDroite - Diagonale HautDroite

    coord dirSearch[4] = {{0, 1},
                              {1, 0},
                              {1, 1},
                              {1, -1}};

    for (int i = 0; i < 4; i++) {
        if (verifyPawn(game, dirSearch[i], coordinate, player)) {
            return 1;
        }
    }
    return 0;
}
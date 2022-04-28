#ifndef POWER4SERVER_GAMELOGIC_H
#define POWER4SERVER_GAMELOGIC_H

#define NLINE 6
#define NCOL 7

typedef struct coord {
    int x;
    int y;
} coord;

typedef int Games[NCOL][NLINE];

/* METHODS */
int fullColumn(Games game, int col);
int collumnFull(Games game);
int initgame(Games game);
int playPawn(Games game, int col, int player);
int pawnWins(Games game, coord coordinate, int player);
int verifyPawn(Games game, coord sens, coord position, int player);


#endif //POWER4SERVER_GAMELOGIC_H
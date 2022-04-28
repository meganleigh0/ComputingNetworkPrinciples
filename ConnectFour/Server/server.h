#ifndef POWER4SERVER_SERVER_H
#define POWER4SERVER_SERVER_H

#include "GameLogic.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define PORT    2666
#define MAX_CLIENTS 	10
#define MAX_CLIENTS_game 2
#define MAX_GAMES 5

#define BUF_SIZE 1024



typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

typedef struct Client
{
    SOCKET sock;
    char name[BUF_SIZE];
    int id;
    int component;
}Client;

typedef struct
{
    Client p1;
    Client p2;
    int componentID;
    int componentN;
    int winner;
    pid_t pid;
}data;


/* CONSTANTS */
const int requireInput = 0;
const int pinUpText = 1;
const int pinUpgame = 2;
const int fingame = 3;

/* MESSAGES */

const char msgWelcome[] = "Welcome!\n";
const char msgFlip[] = "Randomly flipping coin to determine first player.\n";
const char msgFirst[] = "%s is first player.\n"; 
const char msgRound[] = "Round %d. Turn: %s.\n"; 
const char msgReqCol[] = "Choose a column.\n";
const char msgBadCol[] = "Invalid column selcted!\n";
const char msgLast[] = "Please wait...\n";
const char msgWon[] = "%s is winner!\n"; 
const char msgTie[] = "Tie !\n";
const char msgHome[] = "////////////////// Four In A Line ////////////////////";
const char msgLobHome[] = "A : Refresh Players.  Q : Disconnect  1 - %d : Select an opponent.\n\nList of Players : \n"; //%d = j
const char msgCap[] = "Too many plays in progress, please wait.\n";
const char msgBye[] = "Thank you for playing !";
const char startgame[] = "STARTGAME";
const char msgConnection[] = " connected !\n";

/* Methods */

data component(data dataplayers);

double random_draw();

void serialize_game(Games game, char *buffer);

int communication_clients(Client *list, int typeMessage, const char *output, Games game, char *input);

int communication_client(SOCKET sock, int typeMessage, const char *output, Games game, char *input);

int search_player(Client* clients, int id, int actual);

void message_lobby(Client c, Client *list, int maxlist);

int update_lobby(Client *clients, Client **waiting, int actual, fd_set *ptr);

static void remove_client(Client *clients, int to_remove, int *actual);

int read_client(SOCKET sock, char *buffer);

int write_clients(Client c, Client *clients, int max, char *buffer);

int new_connection(Client c, Client *clients, int max);

int write_client(SOCKET sock, const char *buffer);

int init_connection(void);

void extinction(int sig);

int estFinStr(char i);



#endif 
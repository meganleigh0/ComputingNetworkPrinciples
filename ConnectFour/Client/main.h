#include "display.h"

#ifndef Client_Main_H
#define Client_Main_H

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define PORT	 2666

#define BUF_SIZE 1024

#define requireInput 0
#define pinUpText 1
#define pinUpgame 2
#define fingame 3


typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;


int init_connection(const char *adresse);
void end_connection(int sock);
int reading_server(SOCKET sock, int *loop);
void write_server(SOCKET sock, const char *buffer);
static int read_server(SOCKET sock, char *buffer);

const char startgame[] = "STARTGAME";



int main();

#endif //Client_Main_H
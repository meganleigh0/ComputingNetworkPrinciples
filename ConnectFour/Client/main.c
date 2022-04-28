#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "main.h"
#include "InputManage.h"


int init_connection(const char *adresse)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin;

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    memset(&sin, 0, sizeof(struct sockaddr_in));
    inet_aton(adresse, &sin.sin_addr);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (connect(sock, (SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()\n");
        exit(errno);
    }

    return sock;
}


void end_connection(int sock)
{
    close(sock);
    printf("Closing conection.\n");
}


int reading_server(SOCKET sock, int *loop)
/* BLOCKING MODE of communication with the server
 * 3 communication modes:
 * - upText = Displays the text sent by the server
 * - upgame = Display of the game board in the console
 * - RequestInput = Displays the text from the server THEN takes a keyboard input which will be sent to the server
 * - Fingame = Show server message and exit BLOCKING INPUT loop
 */
{
    Games game = {0};
    char buffer[BUF_SIZE];
    int n = read_server(sock, buffer);
    int typeMessage = (buffer[0] - '0'); // First character of the message from the server defines the type of message (among 4)

    switch (typeMessage)
    {
        case 3: //Finish game
            *loop = 0;

        case 1: //pinUp Text
            strcpy(buffer, substring(buffer, 1, (int) strlen(buffer)));
            printf("%s\n", buffer);
            break;

        case 0://Require input
            strcpy(buffer, substring(buffer, 1, (int) strlen(buffer)));
            printf("%s\n", buffer);
            char output[BUF_SIZE];
            reading(output, BUF_SIZE - 1);
            write_server(sock, output);
            break;

        case 2: //pinUp game
            deserialize_game(substring(buffer, 1, (int) strlen(buffer)), game);
            pinUp(game);
            break;

        default:
            printf("Error reading message type.\n");
            end_connection(sock);
            break;
    }

    return n;
}


static int read_server(SOCKET sock, char *buffer)
{
    int n = 0;

    if ((n = (int) recv(sock, buffer, BUF_SIZE, 0)) < 0)
    {
        perror("read() error");
        exit(errno);
    }

    buffer[n] = '\0'; // End the string with an EOF

    return n;
}

void write_server(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, BUF_SIZE, 0) < 0)
    {
        perror("send()");
        exit(errno);
    }

}


int main()
{
    char addressserver[] = "127.0.0.1"; //Server IP Address
    char pseudo[30]; // Pseudo (max 29 characters)

    fd_set rdfs; // Descriptor for select() function

    char buffer[BUF_SIZE];

    // Retrieve name 
    printf("Please enter your nickname. \n");
    if (!reading(pseudo, sizeof(pseudo)))
    {
        printf("Error entering nickname : %s \n", pseudo);
        return EXIT_FAILURE;
    }

    // CONNECTION
    SOCKET sock = init_connection(addressserver);
    // Send name 
    write_server(sock, pseudo);


    while (1)
    {
        FD_ZERO(&rdfs); // Reset select() descriptor

        // Add STDIN (keyboard input) to descriptor
        FD_SET(STDIN_FILENO, &rdfs);

        // Add Socket (server) to descriptor
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            /*  STDIN = keyboard input => send to server
            * SOCK = Receive from server => Display
            * ==> Refreshing the list of pending players
            * ==> Disconnect from server
            * ==> Starting a game with a player from the lobby
            *  play = end of a game, reception of results
            */
            perror("select()");
            exit(errno);
        }

        // Keyboard input event
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            fgets(buffer, BUF_SIZE - 1, stdin);
            {
                char *p = NULL;
                p = strstr(buffer, "\n"); // If no \n at the end, we will add a \0 at the end of the buffer
                if (p != NULL)
                {
                    *p = 0;
                } else
                {
                    // clean 
                    buffer[BUF_SIZE - 2] = '\0';
                }
            }
            write_server(sock, buffer); // Send to server
        } else if (FD_ISSET(sock, &rdfs))
        {
            // Recieved from server event
            int n = read_server(sock, buffer);
            /* server down */
            if (n == 0)
            {
                printf("Server disconnected! \n");
                end_connection(sock);
                return EXIT_SUCCESS;
            }
            if (strcmp(buffer, startgame) == 0) // If the server sends STARTGAME, ==> BLOCKING MODE of communication
            {
                FD_ZERO(&rdfs); // Delete the descriptors of the select()
                sleep(1); // Wait 1 sec for it to be done
                int loopGame = 1;
                while (loopGame)
                {
                    if ((reading_server(sock, &loopGame) == -1))
                    { // Reading/Writing BLOCKING MODE (in a game)
                        printf("Server disconnected! \n");
                        loopGame = 0;
                    }
                }

            } else puts(buffer); // If the message from the server != STARTGAME, display
        }
    }


    return EXIT_SUCCESS;
}
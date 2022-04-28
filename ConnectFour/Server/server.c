#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/wait.h>


#include "server.h"

data component(data dataplayers)
{
    char buffer[BUF_SIZE];
    int currentPlayer; // Designate which player is taking this turn
    Games game = {0}; // power 4 game board
    initgame(game); // Initialize game
    coord pawn = {0, 0}; // coordinates of the played move
    int round = 1; // Game round
    data dataend = dataplayers;
    dataend.componentID = 666;

    Client players[2] = {dataplayers.p1, dataplayers.p2}; // Retrieve the data needed for the game

    printf("Start Game n%d.\n", dataplayers.componentID);

    for (int i = 0; i < MAX_CLIENTS_game; i++)
    {
        write_client(players[i].sock, startgame); // BLOCKING MODE of input for the client
    }

     /*------------------------Welcome message to players ------------------ --------*/
    if (communication_clients(players, pinUpText, msgWelcome, NULL, NULL) == -1) return dataend;
    if (communication_clients(players, pinUpText, msgFlip, NULL, NULL) == -1) return dataend;

    /*---------------- draw of the first player -----------------------*/   
    if (random_draw() > 0.5) currentPlayer = 0;
    else currentPlayer = 1;
    sprintf(buffer, msgFirst, players[currentPlayer].name);
    if (communication_clients(players, pinUpText, buffer, NULL, NULL) == -1) return dataend;

     /*--------------------- Game loop----------------------- ----*/
    while (!pawnWins(game, pawn, currentPlayer) && !collumnFull(game)) // Until the game board is full or won
    {

        if (round != 1) currentPlayer ^= 1;

         // ----------- DISPLAY ALL PLAYERS -----------------
        if (communication_clients(players, pinUpgame, NULL, game, NULL) == -1) return dataend;
        // Display the round
        sprintf(buffer, msgRound, round, players[currentPlayer].name);
        if (communication_clients(players, pinUpText, buffer, NULL, NULL) == -1) return dataend;

        // ----------- player CURRENT PLAYING play -----------------
        // ------------------------------------------------ ------
        // Loop to ask the COLUMN where to play the play
        // if the given COLUMN is not valid (out of game or full),remain in the loop
        int pawnOK = 0;
        int posx;

        do
        {
            // Column input request
            if (communication_client(players[currentPlayer].sock, requireInput, msgReqCol, NULL,
                                     buffer) == -1)
                return dataend;
            posx = (int) (strtol(&buffer[0], NULL, 10) - 1); // Input to int

            if ((posx <= 6 && posx >= 0) && !fullColumn(game, posx))
            {
                pawn.x = posx;
                pawnOK = 1; // validation of players play of pawn
            } else
            {
                if (communication_client(players[currentPlayer].sock, pinUpText, msgBadCol, NULL, NULL) ==
                    -1)
                    return dataend;
            }
        } while (!pawnOK);

        pawn.y = playPawn(game, pawn.x, currentPlayer); // The play is played, retrieve its LINE
        round++; // Increment round counter
    }

     /*--------------------------- END OF GAME (BOARD FULL OR WINNER ------------- ----*/
    if (communication_clients(players, pinUpgame, NULL, game, NULL) == -1) return dataend;

    if (pawnWins(game, pawn, currentPlayer)) // If current player is winner
    {
        sprintf(buffer, msgWon, players[currentPlayer].name);
        if (communication_clients(players, pinUpText, buffer, NULL, NULL) == -1)
            return dataend; // Display win message
        dataplayers.winner = currentPlayer; // Register winning player in data

    } else if (collumnFull(game) && !pawnWins(game, pawn, currentPlayer))  // If equality (full board)
    {
        if (communication_clients(players, pinUpText, msgTie, NULL, NULL) == -1)
            return dataend; // Display draw
        dataplayers.winner = -1; // No winner (-1)
    }
    dataplayers.pid = getpid(); // Registration of the child's pid in data 

    if (communication_clients(players, fingame, NULL, NULL, NULL) == -1)
        return dataend; // Sending FIN to players => DYNAMIC MODE of client input

    return dataplayers;
}


double random_draw()
{
    srandom((unsigned int) time(NULL));
    return (random() / (double) RAND_MAX);
}

void serialize_game(Games game, char *buffer)
{
    int y = 0;
    int x = 0;
    for (y = 0; y < NLINE; y++)
    {
        for (x = 0; x < NCOL; x++)
        {
            buffer[y * NCOL + x] = (char) (game[x][y] + '0');
        }
    }
    buffer[y * NCOL + x] = '\0';
}

int communication_clients(Client *list, int typeMessage, const char *output, Games game, char *input)
{
    for (int i = 0; i < MAX_CLIENTS_game; i++)
    {
        if (communication_client(list[i].sock, typeMessage, output, game, input) == -1) return -1;
    }
    return 1;
}

int communication_client(SOCKET sock, int typeMessage, const char *output, Games game, char *input)
{
    char buffer[BUF_SIZE];
    char concat[BUF_SIZE - 1];
    int n;
    buffer[0] = (char) (typeMessage + '0');
    buffer[1] = '\0';
    if (typeMessage == pinUpgame)
    {
        serialize_game(game, concat);
        strcat(buffer, concat);
    } else if (typeMessage == fingame) strcat(buffer, msgLast);
    else strcat(buffer, output);
    n = write_client(sock, buffer);


    if (typeMessage == requireInput && n != -1) return read_client(sock, input);
    return n;
}


int search_player(Client *clients, int id, int actual)
{
    for (int i = 0; i <= actual; i++)
    {
        if (clients[i].id == id) return i;
    }
    return -1;
}

void message_lobby(Client c, Client *list, int maxlist)
{
    char buffer[BUF_SIZE];
    char temp[BUF_SIZE];
    sprintf(buffer, msgLobHome, maxlist);
    for (int i = 0; i < maxlist; i++)
    {
        if (strcmp(c.name, list[i].name) == 0) sprintf(temp, "%d - /// %s ///\n", i + 1, list[i].name);
        else sprintf(temp, "%d - %s\n", i + 1, list[i].name);
        strcat(buffer, temp);
    }
    write_client(c.sock, buffer);

}

int update_lobby(Client *clients, Client **waiting, int actual, fd_set *ptr)
{
    int i;
    int j = 0;
    for (i = 0; i < actual; i++)  // Add non-game clients to the list of players in the lobby
    {

        if (clients[i].component == -1) // If the player is not in a game, he will be in the lobby (list waiting)
        {
            waiting[j] = &clients[i]; // Fill in the pending players variable
            FD_SET(waiting[j]->sock, ptr);
            j++;
        }
    }
    return j;

}

static void remove_client(Client *clients, int to_remove, int *actual)
{

/* we remove the client in the array */

    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));

/* number client - 1 */

    *actual -= 1;
}

int read_client(SOCKET sock, char *buffer)
{
    int n = 0;

    if ((n = (int) recv(sock, buffer, BUF_SIZE, 0)) < 0)
    {
        perror("read()");
        // if error, disconnect the client
        close(sock);
        printf("Client disconnected!\n");
        return -1;
    }
    buffer[n] = '\0';

    return n;
}

int write_clients(Client c, Client *clients, int max, char *buffer)
{
    char message[BUF_SIZE];
    sprintf(message, "%s : ", c.name);
    strcat(message, buffer);

    for (int i = 0; i < max; i++)
    {
        if (clients[i].id != c.id)
        {
            if (write_client(clients[i].sock, message) == -1)
            {
                remove_client(clients, i, &max);
                return -1;
            }
        }
    }
    return 1;
}

int new_connection(Client c, Client *clients, int max)
{
    char message[BUF_SIZE];
    sprintf(message, "%s ", c.name);
    strcat(message, msgConnection);

    for (int i = 0; i < max; i++)
    {
        if (clients[i].id != c.id)
        {
            if (write_client(clients[i].sock, message) == -1)
            {
                remove_client(clients, i, &max);
                return -1;
            }
        }
    }
    return 1;
}

int write_client(SOCKET sock, const char *buffer)
{
    int n = (int) send(sock, buffer, BUF_SIZE, 0);
    if (n == -1)
    {
        fprintf(stderr, "Error function->write_client: write");
        close(sock);
        return -1;
    }
    return n;
}


int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin;

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listn()");
        exit(errno);
    }

    return sock;
}

void extinction(int sig)
{
    exit(sig);
}

int estFinStr(char i)
{
    if (i == '\n' || i == '\0' || i < 0) return 1;
    return 0;
}

int main()
{
    SOCKET sock = init_connection(); // TCP server initialization
    // Variables

    Client clients[MAX_CLIENTS]; // List of all clients
    Client *waiting[MAX_CLIENTS]; // List of clients who are not in a game
    int id = 0; // Unique id for each player

    int actual = 0; // Number of connected clients
    int max = sock; 

    // Variables of plays
    int componentID = 0; // Unique id for each game
    int pip[MAX_GAMES][2];
    int a;
    for( a = 0; a < MAX_GAMES; a = a + 1 ){
        int b;
      for( b = 0; b < 3; b = b + 1){
          pip[a][b] = -1;
      }
    }
    int componentN = 0; 
    data components[MAX_GAMES]; 
    FILE *log = NULL; // descriptor to target game scores 

    char buffer[BUF_SIZE];

    fd_set rdfs; // reading descriptor for select() function


    /* ------------------- SIGNAL PREPARATION ------------------------- -------*/
    signal(SIGTERM, extinction);


    /*---------------------LOBBY (WAITING ROOM)-------------------- ----------*/
    while (1)
    {
        int i; // control for loops
        int j; // count players in lobby/outside of a game

        FD_ZERO(&rdfs);  // Zeroing the reading descriptor

        FD_SET(sock, &rdfs); // Add sock to the reading descriptor for connecting clients

        j = update_lobby(clients, waiting, actual, &rdfs);

        for (i = 0; i < MAX_GAMES; i++) 
        {
            if (pip[i][0] > -1)
            {
                FD_SET(pip[i][0], &rdfs);
                printf("Select() on pip%d\n", i);
                max = pip[i][0] > max ? pip[i][0] : max;
            }

        }

        /*-------------------EVENT SELECTOR ------------------------ ---------------*/

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) // Select function
            /* CAS SOCK = client connection
             * CLIENT SOCK CASE = Client keyboard input
             * ==> Updating the list of pending players
             * ==> Disconnect from server
             * ==> Starting a game with a player from the lobby
             * CASE play = end of a game, reception of results, extinction of the son
             */
        {
            perror("select()");
            exit(errno);
        } else if (FD_ISSET(sock, &rdfs))
        {

           /* ------------ NEW CLIENT CONNECTION ------------------*/
            SOCKADDR_IN csin;
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *) &csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()\n");
                continue;
            }

            /* after connection, the client sends nickname */
            if (read_client(csock, buffer) == -1)
            {
                /* Disconnection */
                continue;
            }
            puts(buffer);

            /* calculate maximum */
            max = csock > max ? csock : max;
            printf("new max\n");
            FD_SET(csock, &rdfs);

            /* save the new client in the client list */
            Client c = {csock};
            strncpy(c.name, buffer, BUF_SIZE - 1);
            c.component = -1;
            c.id = id;
            clients[actual] = c;

            actual++;
            id++;

            printf("new client, name = %s\n", c.name);

             /* save in the waiting list */
            j = update_lobby(clients, waiting, actual, &rdfs);


            write_client(c.sock, msgHome);
            message_lobby(c, *waiting, j); // display of the welcome message and the list of players to the client
            new_connection(c, clients, actual);

        } else
        {
            for (i = 0; i < j; i++) // Review all clients not playing from the waiting list
            {
                /* ------------ SENDING A MESSAGE TO LOBBY CUSTOMER ------------------*/
                if (FD_ISSET(waiting[i]->sock, &rdfs))
                {
                    printf("Read user keyboard\n");
                    Client client = *waiting[i];
                    int posClient = search_player(clients, client.id,
                                                      actual); // position of the client in the list clients[]

                    int c = read_client(client.sock, buffer); // writing to the buffer
                    if (c == 0) // if empty, error => client disconnected => deletion of client
                    {
                        close(client.sock);
                        remove_client(clients, posClient, &actual); // remove client from client list
                    } else
                    {

                        /* ------------ INPUT = A ==> UPDATE ------------------*/
                        if ((buffer[0] == 'A' || buffer[0] == 'a') && (estFinStr(buffer[1])))
                        {
                           // Lobby message with list player
                            message_lobby(client, *waiting, j);


                        } else if ((buffer[0] == 'Q' || buffer[0] == 'q') && (estFinStr(buffer[1])))
                        {
                            /* ------------ INPUT = Q ==> DISCONNECTION  ------------------*/

                            write_client(client.sock, msgBye);
                            close(clients[posClient].sock);
                            sprintf(buffer, "%s has disconnected !\n", client.name);
                            write_clients(client, *waiting, j, buffer);
                            remove_client(clients, posClient, &actual);
                        } else
                        {
                            /* ------------ INPUT = Number ==> play COUNTER Number ------------------*/

                            int isnumber = 0;
                            for (int k = 0; k < strlen(buffer); k++)
                            {
                                if (!isdigit(buffer[k])) isnumber++;
                            }
                            if (isnumber == 0)  
                            {
                                long choix = strtol(buffer, NULL, 10) - 1; // convert to long

                                if (choix == i)
                                {
                                    sprintf(buffer, "You cannot choose yourself! \n");
                                    write_client(client.sock, buffer);
                                    message_lobby(client, *waiting, j); // display of the welcome message and the list of players to the client

                                } else if (choix >= 0 && choix < j) 
                                {
                                    /* ------------ SETUP play ------------------*/

                                    /* Check that there are still free spots */
                                    componentN = -1;
                                    for (int t = MAX_GAMES - 1; t >= 0; t--)
                                    { if (pip[t][0] < 0) componentN = t; }

                                    if (componentN > -1) // At least one free spot to create a play
                                    {
                                        waiting[i]->component = componentID; // Update the player's play => leave the lobby
                                        waiting[choix]->component = componentID; // Update the player's play => leave the lobby


                                        /* Remove the socks of the 2 players from the list of the select() descriptor */
                                        FD_CLR(waiting[i]->sock, &rdfs);
                                        FD_CLR(waiting[choix]->sock, &rdfs);


                                        /* Data to pass to the play(data a) method */
                                        components[componentID].p1 = *waiting[i];
                                        components[componentID].p2 = *waiting[choix];
                                        components[componentID].componentID = componentID;
                                        components[componentID].componentN = componentN;


                                        /* Create the play */
                                        if (pipe(pip[componentN]) == -1)
                                        {
                                            printf("pipe error\n");
                                            return 1;
                                        }

                                        /* FORK */
                                        int pid;
                                        if ((pid = fork()) < 0)
                                        {
                                            printf("fork error\n");
                                            return 2;
                                        }

                                        if (pid == 0)
                                        {
                                            /* child */
                                            close(pip[componentN][0]);// close Read on play

                                            data resultats = component(
                                                    components[componentID]); // play and recovery results

                                            write(pip[resultats.componentN][1], &resultats,
                                                  sizeof(resultats)); // sending results on play

                                            sleep(3); // time the parent can read 
                                            close(pip[resultats.componentN][1]); // play closure
                                            exit(EXIT_SUCCESS); // child extinction

                                        } else
                                        {
                                            /* parent */
                                            close(pip[componentN][1]); // close Write to play
                                            componentID++; // increment play ID

                                        }


                                    } else // More play lines available
                                    {
                                        strcpy(buffer, msgCap);
                                        write_client(client.sock, buffer);
                                    }
                                }
                            } else
                            { // another input from the lobby client
                                /* ------------------------ CHAT FUNCTION ---------------------- -------*/
                                puts(buffer);
                                write_clients(client, *waiting, j,
                                              buffer); // Send message to other lobby clients (chat)
                            }
                        }
                    }
                }
            }
            for (i = 0; i < MAX_GAMES; i++)
            {
               /*------------------- EXIT CHECK play = results of a play ----------*/
                /* If a play is activated by the end of a child process (play) */
                if (pip[i][0] > -1)
                {
                    if (FD_ISSET(pip[i][0], &rdfs)) // reading descriptior
                    {
                        int status;
                        data resultsParent;
                        read(pip[i][0], &resultsParent, sizeof(resultsParent)); // recovery on play output

                        int idJ1 = search_player(clients, resultsParent.p1.id, actual);
                        int idJ2 = search_player(clients, resultsParent.p2.id, actual);

                        close(pip[i][0]); // closing of play
                        waitpid(resultsParent.pid, &status, 0); // clean closure
                        pip[i][0] = -1; // play set to zero (for the conditions used above)
                        printf("%s on %s\n", resultsParent.p1.name, resultsParent.p2.name);

                        if (resultsParent.componentID == 666)
                        {
                            close(resultsParent.p1.sock);
                            close(resultsParent.p2.sock);
                            remove_client(clients, idJ1, &actual);
                            remove_client(clients, idJ2, &actual);
                        } else
                        {

                            // Clients are now out of play. Added to waiting room
                            clients[idJ1].component = -1;
                            clients[idJ2].component = -1;
                            j = update_lobby(clients, waiting, actual, &rdfs);
                            write_client(clients[idJ1].sock, msgHome);
                            message_lobby(clients[idJ1], *waiting, j);
                            write_client(clients[idJ2].sock, msgHome);
                            message_lobby(clients[idJ2], *waiting, j);



                             /* --------------- WRITE THE SCORE TO A PERMANENT LOG FILE -----------------*/
                            log = fopen("Score.log", "a"); // a = Write at end of file

                            if (log != NULL)
                            {
                                Client player[MAX_CLIENTS_game];
                                player[0] = resultsParent.p1;
                                player[1] = resultsParent.p2;

                                buffer[0] = '\0';
                                char temp[36];
                                for (int f = 0; f < MAX_CLIENTS_game; f++)
                                {
                                    if (resultsParent.winner == f) sprintf(temp, " - [%s]", player[f].name);
                                    else sprintf(temp, "- %s ", player[f].name);
                                    strcat(buffer, temp);

                                }

                                fprintf(log, "game n.%d%s\n", resultsParent.componentID, buffer);
                                fclose(log);
                            }
                        }
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
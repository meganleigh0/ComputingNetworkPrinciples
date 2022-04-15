#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for exit() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include "message.h" 

void HandleTCPBroker(int clntSocket, int client_key, int pub_key, int priv_key)
{

    struct BtoC req;                    // Principle to principle 
    int stock_e;                    // Encrypted trade 
    int stock_d;                       // Decrypted trade 
    int msgLen;                        // Size of message received

    /* Receive message from client TCP */
    if ((msgLen = recv(clntSocket, &stock_e, sizeof (int), 0)) < 0)
        DieWithError("recv() failed");
    stock_d = Dec(stock_e, priv_key);
    printf("Received message from client\n");

    printf("Trading");

    // Return encrypted trade with verification
    printf("Sending verification\n");
    stock_e = Enc(stock_d, client_key);
    if (msgLen = send(clntSocket, &stock_e, sizeof (int), 0) < 0)
        DieWithError("Failed to send verification");
    printf("Sent.\n");
    
    // Receive verification
    printf("Waiting for verification...\n");
    if ((msgLen = recv(clntSocket, &stock_e, sizeof (int), 0)) < 0)
         DieWithError("recv() failed");
    stock_d = Dec(stock_e, client_key);
    if (stock_e > 999)
        DieWithError("Verification failed");
    printf("Verification Received.\n");
    
    // Send signal to verify completion
    printf("Trade successful, sending done key\n");
	stock_d += 1000;
    stock_e = Enc(stock_d, client_key);
    if (msgLen = send(clntSocket, &stock_e, sizeof (int), 0) < 0)
        DieWithError("Failed to send done message");
    printf("Sent. Done\n");
    
}

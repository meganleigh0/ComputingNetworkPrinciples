#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>
#include <sys/types.h> /* definitions for data types in system calls */
#include <sys/socket.h> /* for recv() and send() */
#include <netinet/in.h> //Constants and structures for Internet domain addresses
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "message.h" /* for struct defs */
#include "functions.h"

#define PORT 27403
#define MAXPENDING 5    /* Maximum outstanding connection requests */

void HandleTCPBroker(int clntSocket, int client_key, int pub_key, int priv_key);

int main(int argc, char *argv[]) {
    struct sockaddr_in broker_addr;
    struct sockaddr_in clntAddr;        // Addresses
    struct BCtoKey reg, req;              // Register and request structure
    struct BtoC trade;                   // Principle to principle connection
    int pub_key, priv_key;              // Keys
    int port_num = PORT + 1;                  // Port number
    int client_key;                     // Client key 
    int TCPSockS, TCPSockC;    // Sockets for client server TCP connection
    socklen_t cliAddrLen;               // Received message size 
    
    // Ensure port number was included in argument
    if (argc != 1) {
        DieWithError("Input Incorrect");
    }
    
     /* Construct local address structure */
    memset(&broker_addr, 0, sizeof (struct sockaddr_in)); /* Zero out structure */
    broker_addr.sin_family = AF_INET; /* Internet addr family */
    broker_addr.sin_port = htons(port_num); /* Server port */
    broker_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); /* Server IP address */

    // Generate a public key
    pub_key = KeyGen(&priv_key);

    // Complete register structure
    memset(&reg, 0, sizeof (struct BCtoKey));
    reg.request_type = regista;
    reg.principle_id = 2;
    reg.public_key = pub_key;

    // Register with Key manager
    printf("Registering Broker...");
    SetKey(reg);
    
    // Create TCP port
    printf("Creating TCP port for incoming clients...\n");
    /* Create socket for incoming connections */
    if ((TCPSockS = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("TCP socket() failed");
    /* Bind to the local address */
    if (bind(TCPSockS, (struct sockaddr *) &broker_addr, sizeof (broker_addr)) < 0)
        DieWithError("bind() failed");
    /* Mark the socket so it will listen for incoming connections */
    if (listen(TCPSockS, MAXPENDING) < 0)
        DieWithError("listen() failed");
    printf("Done.\nReady for input\n");

    // Infinite run
    for (;;) {
        // Set address length
        cliAddrLen = sizeof (struct sockaddr_in);
        
        /* wait for client to connect */
	    printf("Ready for client request on port %i...\n", port_num);

        if ((TCPSockC = accept(TCPSockS, (struct sockaddr *) &clntAddr,
                &cliAddrLen)) < 0)
            DieWithError("accept() failed");
        printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));
        
        // Get client public key
        req.principle_id = 1;  //1 for client 2 for broker
        req.request_type = request_key;
        req.public_key = pub_key;
        client_key = GetKey(req);
        
        HandleTCPBroker(TCPSockC, client_key, pub_key, priv_key);
    }
    // Not reached
    return 0;
}


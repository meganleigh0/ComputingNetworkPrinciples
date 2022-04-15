#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>
#include <sys/types.h> // Definitions for system call data types
#include <sys/socket.h> /* for recv() and send() */
#include <netinet/in.h> // Constants and structures for Internet domain addresses
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "message.h" /* for struct defs */
#include "functions.h"

#define PORT 27402

int GetKey(struct BCtoKey req) {

    struct sockaddr_in keymanageAddr;   // Key manager address
    struct KeytoBC info;                  // Structure containing principle public key
    int port_num = PORT;                  // Port
    int sock;                           // UDP Socket
    
    // Key manager local address structure
    memset(&keymanageAddr, 0, sizeof (struct sockaddr_in)); /* Zero out structure */
    keymanageAddr.sin_family = AF_INET; /* Internet address family */
    keymanageAddr.sin_port = htons(port_num); /* Server port */
    keymanageAddr.sin_addr.s_addr = inet_addr("127.0.0.2"); /* Key manager IP address */

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("UDP socket() failed");

    // Send key request
    printf("Sending key request\n");
    if (sendto(sock, &req, sizeof (struct BCtoKey), 0,
            (struct sockaddr *) &keymanageAddr, sizeof (struct sockaddr)) < 0)
        DieWithError("Failed to request client key");

    // Receive key manager response
    if (recv(sock, &info, sizeof (struct KeytoBC), 0) < 0)
        DieWithError("Failed to retrieve client key\n");
    printf("Key received\n");

    close(sock);
    return info.public_key;
}
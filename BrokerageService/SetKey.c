#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>
#include <sys/types.h> // Definitions for data types in system calls
#include <sys/socket.h> /* for recv() and send() */
#include <netinet/in.h> // Constants and structures for Internet domain addresses
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "message.h" /* for struct defs */
#include "functions.h"

#define PORT 27402

void SetKey(struct BCtoKey reg) {
    
    struct sockaddr_in keymanageAddr;   // Key manager Address
    int port_num = PORT;                  // Port number
    int sock;                           // UDP Socket
    
    // Local address structure for key manager
    memset(&keymanageAddr, 0, sizeof (struct sockaddr_in)); /* Zero out structure */
    keymanageAddr.sin_family = AF_INET; /* Internet addr family */
    keymanageAddr.sin_port = htons(port_num); /* Server port */
    keymanageAddr.sin_addr.s_addr = inet_addr("127.0.0.2"); /* Key manager IP address */

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("UDP socket() failed");
    
    // Register principle
    if (sendto(sock, &reg, sizeof (struct BCtoKey), 0,
            (struct sockaddr *) &keymanageAddr, sizeof (struct sockaddr_in)) < 0)
        DieWithError("Registration failed");
    printf("Done.\n");

    close(sock);
}
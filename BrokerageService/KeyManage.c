#include <stdio.h> /* for printf() and fprintf() */
#include <arpa/inet.h>
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h> /* for close() */
#include <stdlib.h>
#include <string.h>
#include "message.h"
#include "functions.h"

#define PORT 27402

int main(int argc, char *argv[]) {
    FILE *pFile; // Store keys
    struct BCtoKey res, req; // Result and request key structures
    struct KeytoBC send_key;
    struct sockaddr_in key_addr, fromAddr;
    int sock; // Socket definition
    int port = PORT;
    int msgLen;
    socklen_t addrLen;

    if (argc != 1) {
        DieWithError("Invalid Number of arguments");
    }

    // Completing server information 
    memset(&key_addr, 0, sizeof (struct sockaddr_in));
    key_addr.sin_family = AF_INET;
    key_addr.sin_addr.s_addr = inet_addr("127.0.0.2");
    key_addr.sin_port = htons(port);

    /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    // Bind socket to local address 
    if (bind(sock, (const struct sockaddr *) &key_addr,
            sizeof (struct sockaddr_in)) < 0)
        DieWithError("bind() failed");
    printf("Ready to receive key request\n");
    
    // Infinite run
    for (;;) {
        memset(&req, 0, sizeof (struct BCtoKey));
        memset(&fromAddr, 0, sizeof (struct sockaddr_in));
        addrLen = sizeof (struct sockaddr_in);
        // Receive message
        if ((msgLen = (recvfrom(sock, &req, sizeof (struct BCtoKey), 0,
                (struct sockaddr *) &fromAddr, &addrLen))) < 0)
            DieWithError("keymanager recvfrom() failed");
        printf("Handling client %s\n", inet_ntoa(fromAddr.sin_addr));
        
        
        
        // Principle requesting to register
        if (req.request_type == regista) {
            printf("Writing key to file...\n");
            pFile = fopen("crypt.txt", "ab"); // Store in file
            fwrite(&req,sizeof(struct BCtoKey),1, pFile);
            printf("Written: %u %i ", req.principle_id, req.public_key);
            fclose(pFile);
            printf("Done\n");
        }
        
        
        
        // Principle requesting key
        if (req.request_type == request_key) {
            pFile = fopen("crypt.txt", "rb");
            printf("Key requested for: %u %i ", req.principle_id, req.public_key);
            printf("Searching for Requested Key\n");
           
            while ( fread(&res,sizeof(struct BCtoKey),1, pFile) == 1 ) {
                printf("Read: %u %i\n", res.principle_id, res.public_key);
                printf("%ld\n", ftell(pFile));
                
                // Client requested, send key
                if (req.principle_id == res.principle_id) {
                    send_key.principle_id = req.principle_id;
                    send_key.public_key = req.public_key;
                    printf("Sending Key...\n");
                    if (sendto(sock, &send_key, sizeof (send_key), 0,
                            (struct sockaddr *) &fromAddr, sizeof (struct sockaddr_in)) != sizeof (send_key))
                        DieWithError("Could not send requested key");
                    printf("Requested key sent\nDone\n");
                    break;
                }

            }
                fclose(pFile);
                printf("End of file reached.\n");
                printf("Ready to receive key request\n");
        }
    }
    // Not reached
    return 0;
}
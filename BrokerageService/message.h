#ifndef MESSAGE_H 
#define MESSAGE_H

struct BCtoKey; /* Broker & Client to Keymanage */
struct KeytoBC; /* Keymanage to Broker & Client */
struct BtoC; /* Broker to Client & Client to Broker */

typedef struct BCtoKey {

    enum {
        regista, request_key
    } request_type; /* same size as an unsigned int */
    unsigned int principle_id; /* client or broker identifier */
    int public_key; /* client’s or broker’s RSA public key */
} BCtoKey; /* an unsigned int is 32 bits = 4 bytes */

typedef struct KeytoBC {
    unsigned int principle_id; /* client or broker identifier */
    int public_key; /* client’s or broker’s RSA public key */
} KeytoBC; /* an unsigned short is 16 bits = 2 bytes */

typedef struct BtoC {

    enum {
        confirm, done
    } request_type; /* same size as an unsigned int */
    unsigned int client_id; /* client identifier */
    unsigned int transaction_id; /* transaction identifier */
    unsigned int num_stocks; /* number of stocks */
} BtoC;


#endif 

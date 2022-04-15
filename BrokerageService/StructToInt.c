#include <stdio.h>
#include "message.h"

// Convert message structure to an int for encryption
int StructToInt(struct BtoC b) {
    int total;
    if (b.request_type == done)
        total+=1000;
	
    total += b.client_id*100;
    total += b.num_stocks*10;
    total += b.transaction_id;
    
    printf("Output Of Struct to int: %i\n", total);
    return total;
}

#include <stdio.h> /* for printf() and fprintf() */

// Square-and-Multiply Algorithm (SMA)
long sma( long x,  long H,  long n);

long Enc(long msg, long e){
    printf("Encrypting: %ld \n", msg);
    long p = 47, q = 31;
    long n = p*q;
    
    long enc = sma(msg,e,n);
    printf("Encrypted Key: %ld \n", enc);
    return enc;
}


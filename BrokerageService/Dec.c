#include <stdio.h> /* for printf() and fprintf() */

// Square-and-Multiply Algorithm (SMA)
long sma( long x,  long H,  long n);

// Decryption, m = (c ^d ) % n
long Dec(long msg, long d) {
    printf("Decrypting: %ld \n", msg);
    long p = 47, q = 31;
    long n = p*q;
    
    long m = sma(msg,d,n);

    printf("Decrypted Key: %ld \n", m);
    return m;
}

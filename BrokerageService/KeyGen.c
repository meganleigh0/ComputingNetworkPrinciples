/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>

int gcd(int a, int b);
long sma( long x,  long H,  long n);

long KeyGen(long *priv) {

    long p = 47, q = 31;    // Prime values
    long e = 27;    
    long totient = (p - 1)*(q - 1);

    // Encrypt value must be less than co-prime
    while (e < totient) {
        if (gcd(e, totient) == 1)
            break;
        else
            e++;
    }

    // Store private key
    *priv = sma(e,-1,totient); 

    printf("Public key: %ld \n", e);
    printf("Private Key: %ld \n", *priv);
    return e;
}

int gcd(int a, int b) {
    if (b == 0)
        return a;
    else
        return gcd(b, a%b);
}

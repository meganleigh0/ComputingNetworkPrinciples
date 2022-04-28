#ifndef powerClient_A_H
#define powerClient_A_H

#include "display.h"

int reading(char *chain, size_t length);
void emptyBuffer();
int deserialize_game(char *buffer, Games game);
char *substring(const char *s, unsigned int start, unsigned int end);



#endif //powerClient_A_H
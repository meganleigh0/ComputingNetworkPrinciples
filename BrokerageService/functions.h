// Header file for functions and structures
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

int KeyGen(int *priv);
void DieWithError(char *errorMessage);
int Enc(int msg, int e);
int Dec(int msg, int d);
int StructToInt(struct BtoC b);
void IntToStruct(struct BtoC *b, int i);
int GetKey(struct BCtoKey req);
void SetKey(struct BCtoKey req);
#endif	


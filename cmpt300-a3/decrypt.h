/*
Project:         Message Passing (assignment 3)
Author:          Weida Pan
student number:  301295773
SFU user name:   lykonp
lecture section: D1
instructor:      Brian G. Booth
TA:              Scott Kristjanson
date: Nov. 3, 2015
*/
#ifndef DECRYPT_H
#define DECRYPT_H

#include <string.h>
#include "memwatch.h"

typedef unsigned long long ull;

//there constant are givin according to the description
extern const int base;
extern const ull N,D;

//transfer a character ch into a digit
int getDigit(char ch);

//transfer a digit dg into a character
char getChar(int dg);

//return square of dg
ull sqr(ull dg);

//exponentiation by squaring which is an efficient exponentiation, and mod n, where c,d,n is givin according to the description
ull modularExp(ull c, ull d, ull n);

//transfer cipher number to plain text number according to the fomular M=C^d mod n and reuturn it
ull getPlainnum(ull dg);

/*
decrypt the input tweets and return a decrypted string
argument tweets is the encrypted tweet
atgument decrypt is the decrypted tweet
no return value
*/
void decrypt(char *tweets, char *decrypt);

#endif

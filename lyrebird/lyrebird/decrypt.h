/*
Project:         A Distributed Flock of lyrebird(assignment 4)
Author:          Weida Pan
student number:  301295773
SFU user name:   lykonp
lecture section: D1
instructor:      Brian G. Booth
TA:              Scott Kristjanson
date: Dec. 1, 2015
*/
#ifndef DECRYPT_H
#define DECRYPT_H

#include <string.h>
#include "memwatch.h"

typedef unsigned long long ull;

//there constant are givin according to the description
extern const int base;
extern const ull N,D;

/*
the only argument is a char type ch
transfer ch into a digit of int type and return it
*/
int getDigit(char ch);

/*
the only argument is a digit dg of int type
transfer dg into a character and return it
*/
char getChar(int dg);

/*the only argument is a dg of ull type
return the square of dg
*/
ull sqr(ull dg);

/*three argument namely corresponding to the following formular
exponentiation by squaring which is an efficient exponentiation, according to the formulor M = c^d mod n
*/
ull modularExp(ull c, ull d, ull n);

/*the only argument is a dg of ull type which is the cipher number
transfer cipher number to plain text number according to the fomular M=C^d mod n and reuturn it
*/
ull getPlainnum(ull dg);

/*
decrypt the input tweets and return a decrypted string
argument tweets is the encrypted tweet
atgument decrypt is the decrypted tweet
no return value
*/
void decrypt(char *tweets, char *decrypt);

#endif

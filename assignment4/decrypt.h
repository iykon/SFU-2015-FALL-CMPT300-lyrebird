/*
Project:         Message Passing (assignment 3)
Author:          Iykon Pan
SFU user name:   lykonp
lecture section: D100
instructor:      Brian G. Booth
TA:              Scott Kristjanson
*/
#ifndef DECRYPT_H
#define DECRYPT_H

#include <string.h>
#include "memwatch.h"

typedef unsigned long long ull;

extern const int base;
extern const ull N,D;

int getDigit(char c);

ull sqr(ull d);

ull modularExp(ull c, ull d, ull n);

char getChar(int d);

ull getPlainnum(ull d);

void decrypt(char *tweets, char *decrypt);

#endif

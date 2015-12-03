/*
Project:         A Distributed Flock of lyrebird(assignment 4)
Author:          Weida Pan
student number:  301295773
SFU user name:   lykonp
lecture section: D1
instructor:      Brian G. Booth
TA:              Scott Kristjanson
date: 			 Dec. 1, 2015
*/
#include "decrypt.h"

/*
input a char of char type
transfer that char into a digit and return
*/
int getDigit(char c){
	switch(c){
		case ' ': return 0;
		case '#': return 27;
		case '.': return 28;
		case ',': return 29;
		case '\'': return 30;
		case '!': return 31;
		case '?': return 32;
		case '(': return 33;
		case ')': return 34;
		case '-': return 35;
		case ':': return 36;
		case '$': return 37;
		case '/': return 38;
		case '&': return 39;
		case '\\': return 40;
		default: return c-'a'+1;
	}
}

/*
input a digit of int type
transfer a digit into a character and return
*/
char getChar(int d){
	switch(d){
		case 0: return ' ';
		case 27: return '#';
		case 28: return '.';
		case 29: return ',';
		case 30: return '\'';
		case 31: return '!';
		case 32: return '?';
		case 33: return '(';
		case 34: return ')';
		case 35: return '-';
		case 36: return ':';
		default: return 'a'+d-1;
	}
}

//return square of input ull type argument c
ull sqr(ull c){
	return c*c;
}

/*
exponentiation by squaring which is an efficient exponentiation, and mod n
input c,d,n according to formulor M=c^d mod n
return M in the formulor
*/
ull modularExp(ull c, ull d, ull n){
	if(d==1)	
		return c;
	else if(d&1)
		return sqr(modularExp(c,d>>1,n))%n*c%n;
	else
		return sqr(modularExp(c,d>>1,n))%n;
}

/*
the only argument is ull type cipher number
transfer cipher number to plain text number according to the fomulor M=C^d mod n
return the plain number
*/
ull getPlainnum(ull c){
	ull plaintextnumber;
	plaintextnumber=modularExp(c%N,D,N);
	return plaintextnumber;
}

/*
decrypt the input tweets and return a decrypted string
argument tweets is the encrypted tweet
atgument decrypt is the decrypted tweet
no return value
*/
void decrypt(char *tweets, char *decrypt){
	int i,j,member;
	ull ciphernumber,plaintextnumber;
	char group[7];//used to store 6 decrypted characters attained from the decrypted plain text number
	//initailization
	i=-1;
	member=0;
	ciphernumber=0;
	group[6]=0;

	while(tweets[++i]){
		if(i%8==7)//ignore every 8th meaningless character
			continue;
		++member;//6 makes up a group, and member denotes the number of digits in current group
		ciphernumber=ciphernumber*base+getDigit(tweets[i]);
		if(member==6){
			member=0;
			plaintextnumber=getPlainnum(ciphernumber);
			ciphernumber=0;
			for(j=5;j>=0;--j){//transfer plain text number to 6 characters
				group[j]=getChar(plaintextnumber%base);//order these characters
				plaintextnumber/=base;
			}
			decrypt=strcat(decrypt,group);
		}
	}
	return;
}

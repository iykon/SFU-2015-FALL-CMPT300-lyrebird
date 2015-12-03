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
#include "commonfunc.h"

/* 
 * function: getcurtime
 * description: get the time stamp of calling time
 * arguments: none
 * return value: char * type, the address of string of time
 * */
char *getcurtime(){
	char *curtime;
	time_t ctm;
	time(&ctm);
	curtime = ctime(&ctm);
	curtime[strlen(curtime)-1] = 0;//eliminate '\n' at the end of string
	return curtime;
}

/*
 * function: sockread
 * description: read char one by one from socket until hit a '\0'
 * arguments: int sockfd: socket file descriptor
 * 			  char *s: a buffer to store read string
 * return value: on success, number of bytes read is returned, on failure, -1 is returned
 * */
int sockread(int sockfd, char *s){
	int len;
	char buf[2];
	len = 0;
	while(read(sockfd, buf, 1) > 0){
		s[len++] = buf[0];
		if(buf[0] == 0)
			return len;
	}
	return -1;
}



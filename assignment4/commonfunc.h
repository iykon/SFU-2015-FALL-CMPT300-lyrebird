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

#ifndef COMMONFUNC_H
#define COMMONFUNC_H

#include <time.h>
#include <unistd.h>
#include <string.h>

/* 
 * function: getcurtime
 * description: get the time stamp of calling time
 * arguments: none
 * return value: char * type, the address of string of time
 * */
char *getcurtime();

/*
 * function: sockread
 * description: read char one by one from socket until hit a '\0'
 * arguments: int sockfd: socket file descriptor
 * 			  char *s: a buffer to store read string
 * return value: on success, number of bytes read is returned, on failure, -1 is returned
 * */
int sockread(int sockfd, char *s);

#endif

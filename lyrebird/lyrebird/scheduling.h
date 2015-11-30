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

#ifndef SCHEDULING_H
#define SCHEDULING_H

#define MAXCORES 32//the max number of cores a computer may have
#define ROUND_ROBIN 1
#define FCFS 2
//when a child process is ready to decrypt, use following protocal
#define CHILD_READY "0"
#define CHILD_READY_LENGTH 2
//when a child process encounters a error, use following protocal
#define CHILD_ERROR "1"
#define CHILD_ERROR_LENGTH 2

#endif

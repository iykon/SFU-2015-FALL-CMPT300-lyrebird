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

/*
 * Project:			Assignment #4: A Distributed Flock of Lyrebirds
 * Author:			Weida Pan
 * student nunmber:	301295773
 * SFU user name: 	lykonp
 * lecture section:	D1
 * instructor:		Brian G. Booth
 * TA:				Scott Kristjanson
 * date:			Dec. 1, 2015
 * */

/*
 * file name: lyrebird.client.c
 * description: This file includes code of parent process and child process
 * 				parent process gets number of CPU on the machine and 
 * 				creates child processes. Parent process connects to server
 *				through internet socket for tasks and distributes them to 
 *				child processes and sends back feedback through socket.
 * */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <decrypt.h>
#include <scheduling.h>
#include "memwatch.h"
#include "msgprotocol.h"

#define MAXLENGTH 1200  // maximum length of file name and tweets
#define MAXCORES 32

const int base = 41;
const ull N=(ull)429443481*10+7,D=1921821779;

char *encfile, *decfile, *buf; // encrypt file name, decrypt file name, buffer for messages

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
 * function: reverse
 * description: reverse a string
 * arguments: char *s: the string to be reversed
 * return type: none
 * */
void reverse(char *s){
	int i;
	char c;
	for(i = 0; i < strlen(s); ++i) {
		c = s[i];
		s[i] = s[strlen(s)-1-i];
		s[strlen(s)-1-i] = c;
	}
}

/*
 * function: itoa
 * description: convert a decimal integer to a string
 * arguments: int n: integer to be converted
 * 			  char *s: store the converted string
 * return value: return that string
 * */
char *itoa(int n, char *s){
	int i;
	i = 0;
	while(n > 0) {
		s[i++] = n%10+'0';
		n /= 10;
	}
	s[i] = 0;
	reverse(s);
	return s;
}

int lyrebird(int pfd, int cfd){//pfd is file decriptor of the pipe parent process write data into, cfd is for child process to write data into
    char *tweets,*decrypted;//use tweets to store a tweet which is to be decrypted, and decrypted to store the decrypted tweet
    char *encfile,*decfile;//encrypted file name and decrypted file name
	char *errmsg;
    FILE *finp, *foutp;//file point for the input file and the output file
    ssize_t rwlen;//return value of read() or write()
    int retv;//return value

    //initialization
    retv = 0;
    encfile = (char *) malloc (MAXLENGTH*sizeof(char));
    decfile = (char *) malloc (MAXLENGTH*sizeof(char));
    tweets = (char *) malloc (MAXLENGTH*sizeof(char));
    decrypted = (char *) malloc (MAXLENGTH*sizeof(char));
	errmsg = (char *)malloc(MAXLENGTH*sizeof(char));

    if(tweets==NULL || decrypted==NULL || encfile==NULL || decfile==NULL){ //fail to malloc
        write(cfd,CHILD_FERROR,MAXLENGTH);
        retv = 1;
    }
    else{
        while(1) {
            write(cfd,CHILD_READY,MAXLENGTH);//notify parent process that it is ready for work
            rwlen = read(pfd,encfile,MAXLENGTH);//block until read something
            if(rwlen==0)//parent process closed the pipe
                break;
            read(pfd,decfile,MAXLENGTH);
            if(access(encfile,F_OK)==-1){//file not fould
				// handle error message
				strcpy(errmsg, "Unable to find file ");
				strcat(errmsg, encfile);
				strcat(errmsg, " in process ");
				strcat(errmsg, itoa(getpid(), tweets));
				write(cfd, CHILD_ERROR, MAXLENGTH);
				write(cfd, errmsg, MAXLENGTH);
            }
            else{//start decrypting
                finp = fopen(encfile,"r");
                if(finp==NULL){ // fail to open file
					//handle error message
					strcpy(errmsg, "Unable to open file ");
					strcat(errmsg, encfile);
					strcat(errmsg, " in process ");
					strcat(errmsg, itoa(getpid(), tweets));
					write(cfd, CHILD_ERROR, MAXLENGTH);
					write(cfd, errmsg, MAXLENGTH);
                    continue;
                }
                foutp=fopen(decfile,"w+");
                if(foutp==NULL){ //fail to open file
					//handle error message
					strcpy(errmsg, "Unable to open file ");
					strcat(errmsg, decfile);
					strcat(errmsg, " in process ");
					strcat(errmsg, itoa(getpid(), tweets));
					write(cfd, CHILD_ERROR, MAXLENGTH);
					write(cfd, errmsg, MAXLENGTH);
                    fclose(finp);
                    continue;
                }
                while(fgets(tweets,MAXLENGTH,finp)!=NULL) {//read an encrypted tweet
                    decrypted[0]=0;
                    decrypt(tweets,decrypted); // call decrypt() function to do the job
                    fprintf(foutp,"%s\n",decrypted);
                }
                fclose(finp);
                fclose(foutp);
				//notify parent process success
				write(cfd, CHILD_SUCCESS, MAXLENGTH);
				write(cfd, encfile, MAXLENGTH);
            }
        }
    }

    free(encfile);
    free(decfile);
    free(tweets);
    free(decrypted);
	free(errmsg);
    return retv;
}

/*
 * function: terminate
 * description: Free memory and exit
 * arguments: int exitv: exit value of program
 * return value: none
 * */
void terminate(int exitv){
	free(buf);
	free(encfile);
	free(decfile);
	exit(exitv);
}

int main(int argc, char **argv){
	char c;
	fd_set rfds; // file descriptor set
	int i,tmp;
	int children; // number of child processes
	int cores; // number of cores
	int nfds; // number of file descriptors
	int pipenum; // pipe number, used in child process
	int sockfd; // socket number of connection to server
	int status; // status of server whether to exit or to work
	int retv; // return value
	int pipepfd[MAXCORES][2], pipecfd[MAXCORES][2];//all pipes used for communication between child processes and parent process
	pid_t endid, pid; // process id
	pid_t childprocess[MAXCORES], childprocess2[MAXCORES]; // both are used to store child processes which are not yet terminated, the latter is an auxiliary one
	struct addrinfo hints;
	struct addrinfo *serverinfo;
	struct sockaddr_in addr;

	// arguments number detection
	if(argc!=3){
		printf("usage: %s <server IP address> <port number>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	encfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	decfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	if(buf == NULL || decfile == NULL || encfile == NULL){
		printf("[%s] Process ID %d failed to allocate memory.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}

	//initialization
	cores = sysconf(_SC_NPROCESSORS_ONLN); // get number of cores
	nfds = 0;
	children = 0;

	// create a socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("[%s] Process ID %d failed to call socket.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	addr.sin_family = AF_INET; // set IPv4
	addr.sin_port = htons(atoi(argv[2])); // set port
	if(inet_pton(AF_INET, argv[1], &addr.sin_addr)<=0) {
		printf("[%s] Process ID %d failed to call inet_pton.\n", getcurtime(), getpid());
		close(sockfd);
		terminate(EXIT_FAILURE);
    } 
	// connect to server
	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0){
		printf("[%s] Process ID %d failed to call connect.\n", getcurtime(), getpid());
		close(sockfd);
		terminate(EXIT_FAILURE);
	}
	printf("[%s] lyrebird client: PID %d connected to server %s on port %s.\n", getcurtime(), getpid(), argv[1], argv[2]);

	//create pipes
	for(i=0;i<cores-1;++i){
        if(pipe(pipepfd[i])==-1){
            printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
			close(sockfd);
			terminate(EXIT_FAILURE);
        }
        if(pipe(pipecfd[i])==-1){
            printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
			close(sockfd);
			terminate(EXIT_FAILURE);
        }
        // get the nfds for select()
        if(nfds<pipepfd[i][0])
            nfds=pipepfd[i][0];
        if(nfds<pipepfd[i][1])
            nfds=pipepfd[i][1];
        if(nfds<pipecfd[i][0])
            nfds=pipecfd[i][0];
        if(nfds<pipecfd[i][1])
            nfds=pipecfd[i][1];
    }
	
	//create child processes
	for(i = 0; i < cores-1; ++i) {
		pid = fork();
		if(pid == -1) {
			printf("[%s] Process ID #%d failed to call fork.\n", getcurtime(), getpid());
			close(sockfd);
			terminate(EXIT_FAILURE);
		}
		else if(pid == 0){ // in child process
			pipenum = i; // the index of pipe to use
			for(i = 0; i < cores-1; ++i) { // close all pipes which is not for this process
				if(i==pipenum){ // find the pipes it uses
                    close(pipepfd[i][1]);
                    close(pipecfd[i][0]);
                }
                else{
                    close(pipepfd[i][0]);
                    close(pipepfd[i][1]);
                    close(pipecfd[i][0]);
                    close(pipecfd[i][1]);
                }
			}
			
			//call lyrebird to decrypt files
			retv = lyrebird(pipepfd[pipenum][0], pipecfd[pipenum][1]);

			close(pipepfd[pipenum][0]);
            close(pipecfd[pipenum][1]);
            free(encfile);
            free(decfile);
			free(buf);
            if(retv)
                 exit(EXIT_FAILURE);
            else
	            exit(EXIT_SUCCESS);
		}
		else{
            childprocess[i] = pid;//record all child process id
            close(pipepfd[i][0]);
            close(pipecfd[i][1]);
		}
	}
	
	/*
	 * communicate with server
	 * get status from server (to do task or to exit)
	 * get file names from server if there is a task
	 * send child process status to server
	 * send error message if any
	 * */
	while(1){
		// handle set of file descriptors
		FD_ZERO(&rfds);
		for(i = 0; i < cores-1; ++i)
			FD_SET(pipecfd[i][0], &rfds);
		// call select waiting for notification
		retv = select(nfds, &rfds, NULL, NULL, NULL);
		if(retv == -1){
			printf("[%s] Process ID #%d failed to call select.\n", getcurtime(), getpid());
			close(sockfd);
			terminate(EXIT_FAILURE);
		}
		else if(retv) {
			// find which pipe has information
			for(i = 0; i < cores-1; ++i)
				if(FD_ISSET(pipecfd[i][0], &rfds))
					break;
			read(pipecfd[i][0], buf, MAXLENGTH);
			// child process is ready for new task
			if(strcmp(buf, CHILD_READY) == 0) {
				// tell server it is ready for new task 
				strcpy(buf, LCREADY);
				write(sockfd, buf, MSGLEN);
				read(sockfd, buf, MSGLEN);
				printf("child ready and get message from server %s.\n", buf);
				// server has more tasks, get file names
				if(strcmp(buf, LSWORK) == 0) {
					read(sockfd, encfile, MAXLENGTH);
					read(sockfd, decfile, MAXLENGTH);
					printf("get work: %s %s\n", encfile, decfile);
					write(pipepfd[i][1], encfile, MAXLENGTH);
					write(pipepfd[i][1], decfile, MAXLENGTH);
				}
				// server has done all works
				else { 
					printf("done\n");
					break;
				}
			}
			// child process decrypted successfully
			else if(strcmp(buf, CHILD_SUCCESS) == 0) {
				// tell server success
				strcpy(buf, LCSUCC);
				write(sockfd, buf, MSGLEN);
				read(pipecfd[i][0], buf, MAXLENGTH); // get the decrypted file name
				write(sockfd, buf, MAXLENGTH);
				printf("child success of %s.\n", buf);
			}
			else if(strcmp(buf, CHILD_ERROR) == 0) { // child process encounters an error which can be fixed
				// tell server failure
				strcpy(buf, LCFAIL);
				write(sockfd, buf, MSGLEN);
				read(pipecfd[i][0], buf, MAXLENGTH);
				write(sockfd, buf, MAXLENGTH);
				printf("child failure of %s.\n", buf);
			}
			else { // child process encounters an error which can be fixed
				// tell server failure and disconnect
				printf("fatal error, escape!!!!!\n");
				strcpy(buf, LCFAIL);
				write(sockfd, buf, MSGLEN);
				strcpy(buf, "A fatal error occurred in process ");
				strcat(buf, itoa(childprocess[i], encfile));
				write(sockfd, buf, MAXLENGTH);
				break; // exit
			}
		}
	}

	/*
	 * close pipes for writing
	 * read the remaining message from pipes and report states to server
	 * close pipes for reading
	 * */
	for(i = 0; i < cores-1; ++i) {
		close(pipepfd[i][1]);
		// read remaining messages
		while(read(pipecfd[i][0], buf, MAXLENGTH)){
			if(strcmp(buf, CHILD_SUCCESS) == 0) {
				// report success to server
				strcpy(buf, LCFAIL);
				write(sockfd, buf, MSGLEN);
				read(pipecfd[i][0], buf, MAXLENGTH);
				write(sockfd, buf, MAXLENGTH);
			}
			else if(strcmp(buf, CHILD_ERROR) == 0 || strcmp(buf, CHILD_FERROR) == 0) { // child process encounters an error
				// report failure to server
				strcpy(buf, LCFAIL);
				write(sockfd, buf, MSGLEN);
				read(pipecfd[i][0], buf, MAXLENGTH);
				write(sockfd, buf, MAXLENGTH);
			}
		}
		close(pipecfd[i][0]);
	}

	// wait until all child processes ternimates
	children = cores-1;
	while(children) {
		tmp = children;
		children = 0;
		for(i=0;i<tmp;++i)
            childprocess2[i] = childprocess[i];
        for(i = 0; i<tmp; ++i){
			endid = waitpid(childprocess2[i], &status, WNOHANG|WUNTRACED);
            if(endid==-1){// error calling waitpid
                printf("[%d] Fail to call waitpid().\n",childprocess2[i]);
				close(sockfd);
				terminate(EXIT_FAILURE);
            }
            else if(endid==0){//child still running
            	childprocess[children++] = childprocess2[i];//put the child process back into the queue, check it later
            }
            else{//child ended
            }
		}
    }
	printf("[%s] lyrebird client: PID %d completed its tasks and is exiting successfully.\n", getcurtime(), getpid());
	close(sockfd);
	terminate(EXIT_SUCCESS);
}

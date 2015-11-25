/*
Project:         Process Manager (assignment 2)
Author:          Iykon Pan
student number:  301295773
SFU user name:   lykonp
lecture section: D1
instructor:      Brian G. Booth
TA:              Scott Kristjanson
date: Oct. 2, 2015
*/
#include <stdio.h>
#include "decrypt.h"
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include "memwatch.h"

#define MAXFILES 100000 // the max number of files
#define MAXCORES 16
#define AXLENGTH 1026

const int base=41;
const int MAXLENGTH=200; //the max length of a tweet
const ull N=(ull)429443481*10+7,D=1921821779;

/*
the function accessed by child process
used to open a encrypted file and decrypt it and write the result into the decrypted file
*/
void lyrebird(char *filein, char *fileout){
        char *tweets,*decrypted;//use tweets to store a tweet which is to be decrypted, and decrypted to store the decrypted tweet
        FILE *finp, *foutp;//file point for the input file and the output file
        finp=fopen(filein,"r");
	if(finp == NULL){ // error opening
		printf("An error occurred while opening the file %s.\n\r", filein);
		exit(EXIT_FAILURE);
	}
        foutp=fopen(fileout,"w+");
	if(foutp == NULL){ // error opening
		printf("An error occurred while opening the file %s.\n\r", fileout);
		exit(EXIT_FAILURE);
	}
        tweets = (char *) malloc (MAXLENGTH*sizeof(char));
        decrypted = (char *) malloc (MAXLENGTH*sizeof(char));
        while(fgets(tweets,MAXLENGTH,finp)!=NULL){//read an encrypted tweet
                decrypted[0]=0;
                decrypt(tweets,decrypted);
                fprintf(foutp,"%s\n",decrypted);
        }
        free(tweets);
        free(decrypted);
        fclose(finp);
        fclose(foutp);
}

/*
main function of the program
read all the files needed to be decrypted
and create child processes to do the decrypting work
wait for all the child processes to terminate and keep track of their status
*/
int main(int argc, char **argv){
	FILE *fp;//file point the exact file read
	pid_t endid,pid; 
	pid_t childprocess[MAXFILES], childprocess2[MAXFILES];//both are used to store child processes which are not yet terminated
	time_t ctm; // current time
	int i,tmp;
	int children,status; //children is the number of child processes it the parent process created, status is the status of a watied child process
	char *encfile,*decfile;//encfile is the name of encrypted files, decfile in the same way
	char *curtime;//current time, string type
	
	if(argc!=2){//too few arguments
		printf("usage: ./lyrebird <target_file>\n\r");
		return 0;
	}
	
	encfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	decfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	children = 0;
	fp = fopen(argv[1],"r+");
	if(fp == NULL){ // error opening the file
		printf("An error occurred while opening the file %s.\n\r", argv[1]);
		exit(EXIT_FAILURE);
	}
	while(EOF != fscanf(fp,"%s ",encfile)){
		fscanf(fp,"%s ", decfile);
		pid = fork(); //create a child process
		if(pid<0){ //fail to create child process
			time(&ctm);
			curtime = ctime(&ctm);
			curtime[strlen(curtime)-1] = 0;//get the current time 
			printf("[%s] Could not create child process.\n\r",curtime); 
		}
		else if(pid==0){ //in child process
			lyrebird(encfile,decfile);
			time(&ctm);
			curtime = ctime(&ctm);
			curtime[strlen(curtime)-1] = 0;
			printf("[%s] Decryption of %s complete. Process ID #%d Exiting.\n\r",curtime,encfile,getpid());
			free(encfile);
			free(decfile);
			exit(EXIT_SUCCESS);
		}
		else{ // in parent process
			time(&ctm);
			curtime = ctime(&ctm);
			curtime[strlen(curtime)-1] = 0;
			printf("[%s] Child process ID #%d created to decrypt %s.\n\r",curtime,pid,encfile);
			childprocess[children++] = pid;//push child process id into the queue
		} 
	}
	//keep track of all the child processes created 
	while(children){
		tmp = children;
		children = 0;
		for(i = 0; i<tmp; ++i)//make a copy of the queue of child processes
			childprocess2[i] = childprocess[i];
		for(i = 0; i<tmp; ++i){
			endid = waitpid(childprocess2[i], &status, WNOHANG|WUNTRACED);
			if(endid<0){// error calling waitpid
				time(&ctm);
				curtime = ctime(&ctm);
				curtime[strlen(curtime)-1] = 0;
				printf("[%s] Fail to call waitpid().\n\r",curtime);	
				exit(EXIT_FAILURE);
			}
			else if(endid==0){//child still running
				childprocess[children++] = childprocess2[i];//put the child process back into the queue, check it later
			}
			else{//child ended
				if(WIFEXITED(status)){//terminate normally
					//nothing is going to happen if the child process terminate normally
				}
				else if(WIFSIGNALED(status)){//terminate abnormally
					time(&ctm);
					curtime = ctime(&ctm);
					curtime[strlen(curtime)-1] = 0;
					printf("[%s] Child process ID #%d did not terminate successfully.\n\r",curtime, childprocess2[i]);
				}
			}
		}
	}
	fclose(fp);
	free(encfile);
	free(decfile);
	return 0;
}

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
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "memwatch.h"

#define MAXFILES 100000 // the max number of files
#define MAXCORES 16
#define MAXLENGTH 1026
#define ROUND_ROBIN 1
#define FCFS 2

const int base=41;
//const int MAXLENGTH=200; //the max length of a tweet
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
	//declare
	FILE *fp;//file point the exact file read
	pid_t endid,pid; 
	pid_t childprocess[MAXFILES], childprocess2[MAXFILES];//both are used to store child processes which are not yet terminated
	time_t ctm; // current time
	int i,tmp;
	int children,status,data,schedule; //children is the number of child processes it the parent process created, status is the status of a watied child process
	int cores,pipenum,selectrtv,nfds;
	int pipepfd[MAXCORES][2],pipecfd[MAXCORES][2];
	char *encfile,*decfile,*scheduling;//encfile is the name of encrypted files, decfile in the same way
	char *curtime;//current time, string type
	char *feedback,*item;
	fd_set rfds;//readable file descriptor set
	
	if(argc!=2){//too few arguments
		printf("usage: %s <target_file>\n\r",argv[0]);
		return 0;
	}
	
	encfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	decfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	scheduling = (char *) malloc (sizeof(char)*MAXLENGTH);
	item = (char *) malloc (sizeof(char)*MAXLENGTH);
	feedback = (char *) malloc (sizeof(char)*MAXLENGTH);
	cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("This machine has %d cores.\n",cores);
	children = 0;
	fp = fopen(argv[1],"r+");
	if(fp == NULL){ // error opening the file
		printf("An error occurred while opening the file %s.\n\r", argv[1]);
		exit(EXIT_FAILURE);
	}
	fgets(scheduling,MAXLENGTH,fp);
	printf("%d\n",strcmp(scheduling,"round robin\n"));
	if(strcmp(scheduling,"round robin\n")==0){
		printf("Round robin used.\n");
		schedule = ROUND_ROBIN;
	}
	else if(strcmp(scheduling,"fcfs\n")==0){
		printf("Fcfs used.\n");
		schedule = FCFS;
	}
	else{
		printf("Unknown scheduling algorithm detected.\n");
		exit(EXIT_FAILURE);
	}

	//pipe and fork
	nfds = 0;
	for(i=0;i<cores-1;++i){
		if(pipe(pipepfd[i])==-1){
			perror("pipe");
			exit(EXIT_FAILURE);
    	}
		if(pipe(pipecfd[i])==-1){
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		// to get the nfds for select()
		if(nfds<pipepfd[i][0])
			nfds=pipepfd[i][0];
		if(nfds<pipepfd[i][1])
			nfds=pipepfd[i][1];
		if(nfds<pipecfd[i][0])
			nfds=pipecfd[i][0];
		if(nfds<pipecfd[i][1])
			nfds=pipecfd[i][1];
	}
	printf("nfds:%d\n",nfds);

	for(i=0;i<cores-1;++i){
		pid=fork();
		if(pid==-1){
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if(pid==0){// in child process
			pipenum=i;//the index of pipe to use
			//printf("This is child process #%d, pipe number %d, with fd %d and %d.\n",getpid(),pipenum,pipepfd[i][0],pipecfd[i][1]);
			for(i=0;i<cores-1;++i){//close all other pipes not used
				if(i==pipenum){
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
			write(pipecfd[pipenum][1],"0",1);//notify parent process that it is ready for work
			//child process working area
			while(1){
				read(pipepfd[pipenum][0],encfile,MAXLENGTH);
				//printf("Child Process No.%d read from pipe: %s\n",pipenum,item);
				if(strcmp(encfile,"0")==0)
					break;
				read(pipepfd[pipenum][0],decfile,MAXLENGTH);
				printf("Child Process No.%d read from pipe: %s %s.\n",pipenum,encfile,decfile);
				write(pipecfd[pipenum][1],"0",1);//notify parent process that it is ready for work
			}
			/*while(1){
			read(pipepfd[pipenum][0],item,MAXLENGTH);
			printf("CHILD PROCESS No.%d GET DATA %s\n",pipenum,item);
			sleep(10);
			printf("CHILD PROCESS No.%d done, write to %d\n",pipenum,pipecfd[pipenum][1]);
			write(pipecfd[pipenum][1],"0",1);
				}
			*/
			printf("Child Process No.%d terminates\n",pipenum);
			close(pipepfd[pipenum][0]);
			close(pipecfd[pipenum][1]);
			free(encfile);
			free(decfile);
			free(scheduling);
			free(item);
			free(feedback);
			exit(EXIT_SUCCESS);
		}
		else{//in parent process
			printf("Child process #%d created.\n",pid);
			close(pipepfd[i][0]);
			close(pipecfd[i][1]);
		}
	}
	//parent process
	FD_ZERO(&rfds);//empty readable file descriptor set
	if(schedule == ROUND_ROBIN){
		while(EOF != fscanf(fp,"%s ",encfile)){
			fscanf(fp,"%s ", decfile);
			printf("%s %s\n",encfile,decfile);
		}
		for(i=0;i<cores-1;++i){
			FD_SET(pipecfd[i][0],&rfds);//put files descriptors which receive feedback from child process into the set
		}
		selectrtv=select(nfds,&rfds,NULL,NULL,NULL);//wait until at least one child process is ready
		if(selectrtv==-1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		else if(selectrtv){
			//for(
		}
		/*while(1){
		//scanf("%s",item);
		//printf("%s\n",item);
		for(i=0;i<cores-1;++i){
			FD_SET(pipecfd[i][0],&rfds);//put files descriptors which receive feedback from child process into the set
		}
		selectrtv=select(nfds,&rfds,NULL,NULL,NULL);//wait until at least one child process is ready
		if(selectrtv==-1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		else if(selectrtv){
			for(i=0;i<cores;++i)
				if(FD_ISSET(pipecfd[i][0],&rfds)){
					break;
				}
			read(pipecfd[i][0],feedback,MAXLENGTH);
			if(feedback[0]=='0'){
				printf("one ready child captured\n");
				write(pipepfd[i][1],item,MAXLENGTH);
			}
		}
		}*/
	}
	else{//scheduling algrithm is FSFC
		printf("fuck!\n");
		sleep(1);
		while(EOF != fscanf(fp,"%s ",encfile)){
			fscanf(fp,"%s ", decfile);
			printf("%s %s\n",encfile,decfile);
			for(i=0;i<cores-1;++i){
				FD_SET(pipecfd[i][0],&rfds);//put files descriptors which receive feedback from child process into the set
			}
			selectrtv=select(nfds,&rfds,NULL,NULL,NULL);//wait until at least one child process is ready
			if(selectrtv==-1){//error occurred when calling select
				perror("select");
				exit(EXIT_FAILURE);
			}
			else if(selectrtv){
				for(i=0;i<cores;++i)
					if(FD_ISSET(pipecfd[i][0],&rfds))
						break;
				read(pipecfd[i][0],feedback,MAXLENGTH);
				if(feedback[0]=='0'){//child is ready
					printf("To write into pipe for child process:%s %s.\n",encfile,decfile);
					write(pipepfd[i][1],encfile,MAXLENGTH);
					write(pipepfd[i][1],decfile,MAXLENGTH);
				}
			}
		}
	}

	/*
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
	}*/
	for(i=0;i<cores-1;++i){
		FD_ZERO(&rfds);
		FD_SET(pipecfd[i][0],&rfds);
		selectrtv = select(pipecfd[i][0]+1,&rfds,NULL,NULL,NULL);
		if(selectrtv==-1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		else{
			write(pipepfd[i][1],"0",MAXLENGTH);
		}
	}
	for(i=0;i<cores-1;++i){
		close(pipepfd[i][1]);
		close(pipecfd[i][0]);
	}
	fclose(fp);
	free(encfile);
	free(decfile);
	free(scheduling);
	free(item);
	free(feedback);
	return 0;
}

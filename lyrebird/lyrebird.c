/*
Project:         Message Passing (assignment 3)
Author:          Iykon Pan
student number:  301295773
SFU user name:   lykonp
lecture section: D1
instructor:      Brian G. Booth
TA:              Scott Kristjanson
date: Nov. 3, 2015
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "memwatch.h"
#include "decrypt.h"
#include "scheduling.h"

#define MAXLENGTH 1100

const int base=41;
const ull N=(ull)429443481*10+7,D=1921821779;

/*
return a string of current time, without '\n' at the end
*/
char *getcurtime(){
	char *curtime;
	time_t ctm;
	time(&ctm);
	curtime = ctime(&ctm);
	curtime[strlen(curtime)-1] = 0;//eliminate '\n' at the end of string
	return curtime;
}

/*
the function accessed by child process
used to open a encrypted file and decrypt it and write the result into the decrypted file
*/
int lyrebird(int pfd, int cfd){
        char *tweets,*decrypted;//use tweets to store a tweet which is to be decrypted, and decrypted to store the decrypted tweet
	char *encfile,*decfile;
        FILE *finp, *foutp;//file point for the input file and the output file
	ssize_t readlen;//return value of read()
	int rtv;//return value
	
	rtv = 0;
	encfile = (char *) malloc (MAXLENGTH*sizeof(char));
	decfile = (char *) malloc (MAXLENGTH*sizeof(char));
        tweets = (char *) malloc (MAXLENGTH*sizeof(char));
        decrypted = (char *) malloc (MAXLENGTH*sizeof(char));
	if(tweets==NULL || decrypted==NULL || encfile==NULL || decfile==NULL){//fail to malloc
		printf("[%s] Child process ID #%d failed to call malloc.\n",getcurtime(),getpid());
		write(cfd,CHILD_ERROR,CHILD_ERROR_LENGTH);
		rtv = 1;
		goto lyrebird_terminate;
	}
	
	write(cfd,CHILD_READY,CHILD_READY_LENGTH);//notify parent process that it is ready for work
	//printf("-----------child process ID #%d is ready.\n",getpid());
	while(1){
		printf("child process ID #%d is making progress to read from the pipe.\n",getpid());
		readlen = read(pfd,encfile,MAXLENGTH);
		printf("child process ID #%d read %d bytes from the pipe.\n",getpid(),(int)readlen);
		if(readlen==0)//parent process closed the pipe
			break;
		read(pfd,decfile,MAXLENGTH);
		printf("Child Process ID #%d read from pipe: %s %s.\n",getpid(),encfile,decfile);
		if(access(encfile,F_OK)==-1){//file not fould
			printf("[%s] Child process ID #%d could not find file %s.\n",getcurtime(),getpid(),encfile);
			write(cfd,CHILD_READY,CHILD_READY_LENGTH);
		}
		else{//file found and start decrypting
			finp = fopen(encfile,"r");
			if(finp==NULL){ // fail to open file
				printf("[%s] Child process ID #%d failed to open file %s.\n",getcurtime(),getpid(),encfile);
				write(cfd,CHILD_READY,CHILD_READY_LENGTH);//notify parent process that it is ready for work
				continue;
			}
			foutp=fopen(decfile,"w+");
			if(foutp==NULL){ //fail to open file
				printf("[%s] Child process ID #%d failed to open file %s.\n",getcurtime(),getpid(),decfile);
				write(cfd,CHILD_READY,CHILD_READY_LENGTH);//notify parent process that it is ready for work
				continue;
			}
			while(fgets(tweets,MAXLENGTH,finp)!=NULL){//read an encrypted tweet
				decrypted[0]=0;
				decrypt(tweets,decrypted);
				fprintf(foutp,"%s\n",decrypted);
			}
			printf("[%s] Process ID #%d decrypted %s successfully.\n",getcurtime(),getpid(),encfile);
			write(cfd,CHILD_READY,CHILD_READY_LENGTH);//notify parent process that it is ready for work
		}
	}
	
	lyrebird_terminate:
	free(encfile);
	free(decfile);
        free(tweets);
        free(decrypted);
        fclose(finp);
        fclose(foutp);
	return rtv;
}

/*
main function of the program
create child processes to do the decrypting work
read all the files needed to be decrypted
schedule according to the input
wait for all the child processes to terminate and keep track of their status
*/
int main(int argc, char **argv){
	FILE *fp;//file pointor to the file read
	pid_t endid,pid;//process id
	pid_t childprocess[MAXCORES], childprocess2[MAXCORES];//both are used to store child processes which are not yet terminated, the latter is an auxiliary one
	int i,tmp;
	int children,status,data,schedule; //children is the number of child processes it the parent process created, status is the status of a watied child process
	int cores;//number of cores
	int pipenum;//the index of pipes
	int currentchild;//used for child processes take turns to get files in round robin algorithm
	int rtv,terminateval;//rtv is general int return value, terminateval is whether the process terminates normally or abnormally
	int nfds;//nfds is the first parameter in select() function
	int pipepfd[MAXCORES][2],pipecfd[MAXCORES][2];//all pipes used for communication between child processes and parent process
	char *encfile,*decfile,*scheduling;//encfile is the name of encrypted files, decfile in the same way
	char *feedback;//feedback from child process to parent process
	fd_set rfds;//readable file descriptor set
	
	if(argc!=2){//incorrect number of arguments
		printf("usage: %s <target_file>\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	
	//read input file and handle errors
	if(access(argv[1],F_OK)==-1){
		printf("[%s] Process ID #%d could not find file %s.\n",getcurtime(),getpid(),argv[1]);
		exit(EXIT_FAILURE);
	}
	fp = fopen(argv[1],"r+");
	if(fp == NULL){ // error opening the file
		printf("[%s] Process ID #%d failed to open file %s.\n", getcurtime(), getpid(), argv[1]);
		exit(EXIT_FAILURE);
	}

	//initialization
	encfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	decfile = (char *) malloc (sizeof(char)*MAXLENGTH);
	feedback = (char *) malloc (sizeof(char)*MAXLENGTH);
	scheduling = (char *) malloc (sizeof(char)*MAXLENGTH);
	if(encfile==NULL || decfile==NULL || scheduling==NULL || feedback==NULL){//error malloc
		printf("[%s] Process ID #%d failed to call malloc.\n",getcurtime(),getpid());
		terminateval = 1;
		goto terminate;
	}
	cores = sysconf(_SC_NPROCESSORS_ONLN);
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<this sentence should be deleted.--------------------------------------------------------
	nfds = 0;
	cores = 4;
	children = 0;
	terminateval = 0;
	
	//read scheduling algorithm
	fgets(scheduling,MAXLENGTH,fp);
	if(strcmp(scheduling,"round robin\n")==0)
		schedule = ROUND_ROBIN;
	else if(strcmp(scheduling,"fcfs\n")==0)
		schedule = FCFS;
	else{
		printf("[%s] Process ID #%d detected an unknown scheduling algorithm.\n",getcurtime(),getpid());
		terminateval = 1;
		goto terminate;
	}

	//create pipes
	for(i=0;i<cores-1;++i){
		if(pipe(pipepfd[i])==-1){
			printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
			terminateval = 1;
			goto terminate;
    		}
		if(pipe(pipecfd[i])==-1){
			printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
			terminateval = 1;
			goto terminate;
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
	for(i=0;i<cores-1;++i){
		pid=fork();
		if(pid==-1){//error fork()
			printf("[%s] Process ID #%d failed to create child processes.\n",getcurtime(),getpid());
			terminateval = 1;
			goto terminate;
		}
		else if(pid==0){// in child process
			pipenum=i;//the index of pipe to use
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
			rtv = lyrebird(pipepfd[pipenum][0],pipecfd[pipenum][1]);
			printf("child process ID #%d terminates with value %d.\n",getpid(),rtv);
			
			close(pipepfd[pipenum][0]);
			close(pipecfd[pipenum][1]);
			free(encfile);
			free(decfile);
			free(scheduling);
			free(feedback);
			if(rtv)
				exit(EXIT_FAILURE);
			else
				exit(EXIT_SUCCESS);
		}
		else{//in parent process
			childprocess[i] = pid;//record all child process id
			close(pipepfd[i][0]);
			close(pipecfd[i][1]);
		}
	}

	//in parent process, scheduling
	if(schedule == ROUND_ROBIN){
		currentchild = 0;
		while(EOF!=fscanf(fp,"%s ",encfile)){
			fscanf(fp,"%s ", decfile);
			FD_ZERO(&rfds);//empty readable file descriptor set
			FD_SET(pipecfd[currentchild][0],&rfds);//put the file descriptor which receives feedback from child process into the set
			rtv = select(pipecfd[currentchild][0]+1,&rfds,NULL,NULL,NULL);//wait until that child process is ready
			if(rtv==-1){//error select()
				printf("[%s] Process ID #%d failed to call select().\n",getcurtime(),getpid());
				terminateval = 1;
				goto terminate_with_children;
			}
			else if(rtv){
				read(pipecfd[currentchild][0],feedback,MAXLENGTH);
				if(strcmp(feedback,CHILD_READY)==0){//child is ready
					printf("[%s] Child process ID #%d will decrypt %s.\n",getcurtime(),childprocess[currentchild],encfile);
					write(pipepfd[currentchild][1],encfile,MAXLENGTH);
					write(pipepfd[currentchild][1],decfile,MAXLENGTH);
				}
				else {//child encountered a fatal error
					terminateval = 1;
					goto terminate_with_children;
				}
			}
			++currentchild;//swith to next child process
			if(currentchild==cores-1)
				currentchild=0;
		}
	}
	else{//scheduling algrithm is fcfs
		while(EOF != fscanf(fp,"%s ",encfile)){
			fscanf(fp,"%s ", decfile);
			FD_ZERO(&rfds);//empty readable file descriptor set
			for(i=0;i<cores-1;++i)
				FD_SET(pipecfd[i][0],&rfds);//put files descriptors which receive feedback from child process into the set
			rtv=select(nfds,&rfds,NULL,NULL,NULL);//wait until at least one child process is ready
			if(rtv==-1){//error occurred when calling select
				terminateval = 1;
				goto terminate;
			}
			else if(rtv){
				for(i=0;i<cores;++i)
					if(FD_ISSET(pipecfd[i][0],&rfds))
						break;
				read(pipecfd[i][0],feedback,MAXLENGTH);
				if(strcmp(feedback,CHILD_READY)==0){//child is ready
					printf("[%s] Child process ID #%d will decrypt %s.\n",getcurtime(),childprocess[i],encfile);
					write(pipepfd[i][1],encfile,MAXLENGTH);
					write(pipepfd[i][1],decfile,MAXLENGTH);
				}
				else{//child process encouters a fatal error
					terminateval = 1;
					goto terminate_with_children;
				}
			}
		}
	}

	terminate_with_children:
	//read the remaining messages from the child processes and close pipes
	for(i=0;i<cores-1;++i){
		read(pipecfd[i][0],feedback,MAXLENGTH);//parent process blocks to wait child process to finish the last task and close pipe
		close(pipepfd[i][1]);
		//read the remaining messages from pipe which child process write messages into
		while(read(pipecfd[i][0],feedback,MAXLENGTH));
		close(pipecfd[i][0]);
		close(pipecfd[i][0]);
	}

	//wait until all child processes terminates
	children = cores-1;
	while(children){
		tmp = children;
		children = 0;
		for(i=0;i<tmp;++i)
			childprocess2[i] = childprocess[i];
		for(i = 0; i<tmp; ++i){
                        endid = waitpid(childprocess2[i], &status, WNOHANG|WUNTRACED);
                        if(endid==-1){// error calling waitpid
                                printf("[%d] Fail to call waitpid().\n",childprocess2[i]);
				terminateval = 1;
				goto terminate;
                        }
                        else if(endid==0){//child still running
                                childprocess[children++] = childprocess2[i];//put the child process back into the queue, check it later
                        }
                        else{//child ended
                                if(WIFEXITED(status)){//terminate normally
                                        //nothing is going to happen if the child process terminate normally
                                }
                                else if(WIFSIGNALED(status)){//terminate abnormally
					printf("[%s] Child process ID #%d did not terminate successfully.\n",getcurtime(),childprocess2[i]);
                                }
                        }
                }
	}

	terminate:
	//free and close all memory in the heap
	fclose(fp);
	free(encfile);
	free(decfile);
	free(scheduling);
	free(feedback);
	if(terminateval == 0)
		exit(EXIT_SUCCESS);
	else
		exit(EXIT_FAILURE);
}

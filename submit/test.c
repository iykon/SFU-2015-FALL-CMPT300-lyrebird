#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#define MAXCORES 16
#define MAXLENGTH 1026

int main(int argc, char **argv){
	// declare
	int i,pipenum,childnum,data,nfds,selectrtv;
	int number_of_cores,pipepfd[MAXCORES][2],pipecfd[MAXCORES][2];
	char *item,*feedback;
	pid_t pid;
	fd_set rfds;

	item=(char *)malloc(sizeof(char)*MAXLENGTH);
	feedback=(char *)malloc(sizeof(char)*MAXLENGTH);

	number_of_cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("%d\n",number_of_cores);
	for(i=0;i<number_of_cores-1;++i){
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

	for(i=0;i<number_of_cores-1;++i){
		pid=fork();
		if(pid==-1){
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if(pid==0){// in child process
			pipenum=i;//the index of pipe to use
			printf("This is child process #%d, pipe number %d, with fd %d and %d.\n",getpid(),pipenum,pipepfd[i][0],pipecfd[i][1]);
			for(i=0;i<number_of_cores-1;++i){
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
			write(pipecfd[pipenum][1],"0",1);
			while(1){
			read(pipepfd[pipenum][0],item,MAXLENGTH);
			printf("CHILD PROCESS No.%d GET DATA %s\n",pipenum,item);
			sleep(10);
			printf("CHILD PROCESS No.%d done, write to %d\n",pipenum,pipecfd[pipenum][1]);
			write(pipecfd[pipenum][1],"0",1);
				}
			close(pipepfd[pipenum][0]);
			close(pipecfd[pipenum][1]);
			exit(EXIT_SUCCESS);
		}
		else{//in parent process
			printf("Child process #%d created.\n",pid);
			close(pipepfd[i][0]);
			close(pipecfd[i][1]);
		}
	}
	//parent process
	printf("s\n");
	printf("s\n");
	FD_ZERO(&rfds);
	for(i=0;i<number_of_cores-1;++i){
		FD_SET(pipecfd[i][0],&rfds);
	}
	while(1){
		scanf("%d %s",&childnum,item);
		if(childnum<0 || childnum>=number_of_cores-1)
			break;
		for(i=0;i<number_of_cores-1;++i){
			FD_SET(pipecfd[i][0],&rfds);
		}
		selectrtv=select(nfds,&rfds,NULL,NULL,NULL);
		if(selectrtv==-1){
			perror("select");
			exit(EXIT_FAILURE);
		}
		else if(selectrtv){
			for(i=0;i<number_of_cores;++i)
				if(FD_ISSET(pipecfd[i][0],&rfds)){
					break;
				}
			read(pipecfd[i][0],feedback,MAXLENGTH);
			if(feedback[0]=='0'){
				printf("one ready child captured\n");
				write(pipepfd[i][1],item,MAXLENGTH);
			}
		}
	}

	return 0;
}

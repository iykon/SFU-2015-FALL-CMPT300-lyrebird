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
#include <memwatch.c>
#include <decrypt.h>
#include <scheduling.h>

#define MAXLENGTH 1200
#define MAXLENGTH 1200
#define LSWORK 0x0001
#define LSDONE 0x0002
#define LCSUCC 0x0004
#define LCFAIL 0x0008
#define LCREADY 0x0010
#define MAXCORES 32

const int base = 41;
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

int lyrebird(int pfd, int cfd){//pfd is file decriptor of the pipe parent process write data into, cfd is for child process to write data into
    char *tweets,*decrypted;//use tweets to store a tweet which is to be decrypted, and decrypted to store the decrypted tweet
    char *encfile,*decfile;//encrypted file name and decrypted file name
    FILE *finp, *foutp;//file point for the input file and the output file
    ssize_t rwlen;//return value of read() or write()
    int rtv;//return value

    //initialization
    rtv = 0;
    encfile = (char *) malloc (MAXLENGTH*sizeof(char));
    decfile = (char *) malloc (MAXLENGTH*sizeof(char));
    tweets = (char *) malloc (MAXLENGTH*sizeof(char));
    decrypted = (char *) malloc (MAXLENGTH*sizeof(char));

    if(tweets==NULL || decrypted==NULL || encfile==NULL || decfile==NULL){ //fail to malloc
        printf("[%s] Child process ID #%d failed to call malloc.\n",getcurtime(),getpid());
        write(cfd,CHILD_ERROR,CHILD_ERROR_LENGTH);
        rtv = 1;
    }
    else{
        while(1){
            rwlen = write(cfd,CHILD_READY,CHILD_READY_LENGTH);//notify parent process that it is ready for work
            if(rwlen<0){
                printf("[%s] Child process ID #%d failed to write into pipe.\n",getcurtime(),getpid());
                rtv = 1;
                break;
            }
            rwlen = read(pfd,encfile,MAXLENGTH);//block until read something
            if(rwlen==0)//parent process closed the pipe
                break;
            read(pfd,decfile,MAXLENGTH);
            if(access(encfile,F_OK)==-1){//file not fould
                printf("[%s] Child process ID #%d could not find file %s.\n",getcurtime(),getpid(),encfile);
            }
            else{//start decrypting
                finp = fopen(encfile,"r");
                if(finp==NULL){ // fail to open file
                    printf("[%s] Child process ID #%d failed to open file %s.\n",getcurtime(),getpid(),encfile);
                    continue;
                }
                foutp=fopen(decfile,"w+");
                if(foutp==NULL){ //fail to open file
                    printf("[%s] Child process ID #%d failed to open file %s.\n",getcurtime(),getpid(),decfile);
                    fclose(finp);
                    continue;
                }
                while(fgets(tweets,MAXLENGTH,finp)!=NULL){//read an encrypted tweet
                    decrypted[0]=0;
                    decrypt(tweets,decrypted);
                    fprintf(foutp,"%s\n",decrypted);
                }
                fclose(finp);
                fclose(foutp);
                printf("[%s] Process ID #%d decrypted %s successfully.\n",getcurtime(),getpid(),encfile);
            }
        }
    }

    free(encfile);
    free(decfile);
    free(tweets);
    free(decrypted);
    return rtv;
}

int main(int argc, char **argv){
	char c;
	char *encfile, *decfile, *buf;
	fd_set rfds;
	int i,tmp;
	int children, currentchild;
	int cores, nfds;
	int pipenum;
	int sockfd;
	int status;
	int retv;
	int pipepfd[MAXCORES][2], pipecfd[MAXCORES][2];
	pid_t endid, pid;
	pid_t childprocess[MAXCORES], childprocess2[MAXCORES];
	ssize_t wlen;
	struct addrinfo hints;
	struct addrinfo *serverinfo;
	struct sockaddr_in addr;

	if(argc!=3){
		printf("usage: %s <server IP address> <port number>\n",argv[0]);
		exit(1);
	}

	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	encfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	decfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	if(buf == NULL || decfile == NULL || encfile == NULL){
		printf("[%s] Process ID %d failed to allocate memory.\n", getcurtime(), getpid());
		exit(1);
	}

	/*memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(argv[1],argv[2], &hints, &serverinfo)) != 0){
		printf("getaddrinfo: %s\n", gai_strerror(status));
		exit(2);
	}*/
	cores = sysconf(_SC_NPROCESSORS_ONLN);
	nfds = 0;
	children = 0;
	printf("cores: %d\n",cores);

	//socket connection
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[2]));
	//printf("port:%d\n",atoi(argv[2]));
	if(inet_pton(AF_INET, argv[1], &addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 
	//printf("%d\n",addr.sin_addr.s_addr);
	if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))<0){
		perror("connect");
		exit(1);
	}
	printf("[%s] lyrebird client: PID %d connected to server %s on port %s.\n", getcurtime(), getpid(), argv[1], argv[2]);

	//create pipes
	for(i=0;i<cores-1;++i){
        if(pipe(pipepfd[i])==-1){
            printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
            terminateval = 1;
            //goto terminate;
        }
        if(pipe(pipecfd[i])==-1){
            printf("[%s] Process ID #%d failed to create pipes.\n",getcurtime(),getpid());
            terminateval = 1;
            //goto terminate;
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
			exit(1);
		}
		else if(pid == 0){ // in child process
			pipenum = i; // the index of pipe to use
			for(i = 0; i < cores-1; ++i) {
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
			
			//call lyrebird to decrypt files
			rtv = lyrebird(pipepfd[pipenum][0], pipecfd[pipenum][1]);

			//free
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
		else{
            childprocess[i] = pid;//record all child process id
            close(pipepfd[i][0]);
            close(pipecfd[i][1]);
		}
	}
	
	currentchild = 0;
	while(1){
		FD_ZERO(&rfds);
		for(i = 0; i < cores-1; ++i)
			FD_SET(pipecfd[i][0], &rfds);
		retv = select(nfds, &rfds, NULL, NULL, NULL);
		
		if(retv == -1){
			printf("[%s] Process ID #%d failed to call select.\n", getcurtime(), getpid());
			exit(1);
		}
		else if(retv) {
			for(i = 0; i < cores-1; ++i)
				if(FD_ISSET(pipecfd[i][0], &rfds))
					break;
			read(pipecfd[i][0], buf, MAXLENGTH);
			if(strcmp(buf, CHILD_READY) == 0) {
				status = LCREADY;
				write(sockfd, &status, MAXLENGTH);
				read(sockfd, buf, MAXLENGTH);
				if(buf[0] == LSWORK) {
					read(sockfd, encfile, MAXLENGTH);
					read(sockfd, decfile, MAXLENGTH);
					printf("[%s] Child process ID #%d will decrypt %s.\n", getcurtime(), childprocess[i], encfile);
					write(pipepfd[currentchild][1], decfile, MAXLENGTH);
				}
				else {
					//read remaining messages in pipes
					/*for(j = 0; j < cores-1; ++j) {
						if(i == j)
							continue;
						read(pipecfd[j][0], buf, MAXLENGTH);
					}*/
					//close
					exit(0);
				}
			}
			else{ // child process encounters a fatal error
				
			}
		}
	}
	for(i = 0; i < cores-1; ++i) {
		close(pipepfd[i][1]);
		while(read(pipecfd[i][0], buf, MAXLENGTH));
		close(pipecfd[i][0]);
	}

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
                terminateval = 1;
                goto terminate;
            }
            else if(endid==0){//child still running
            	childprocess[children++] = childprocess2[i];//put the child process back into the queue, check it later
            }
            else{//child ended
            	if(WIFEXITED(status)){}//terminate normally
            	else if(WIFSIGNALED(status)){//terminate abnormally
            		printf("[%s] Child process ID #%d did not terminate successfully.\n",getcurtime(),childprocess2[i]);
            	}
            }
		}
    }
	free(encfile);
	free(decfile);
	free(buf);

	//printf("connect!\n");
	/*retv = read(sockfd, buf, MAXLENGTH);
	if(retv == 0){
		printf("socked closed by server");
		exit(1);
	}
	printf("%s\n",buf);*/
	/*while(1){
		printf("?");
		scanf("%s",buf);
		write(sockfd,"!fail", MAXLENGTH);
		printf("write into socket\n");
	}*/
	/*printf("write\n");
	//strcpy(buf, success);
	status = LCREADY;
	write(sockfd, &status, MAXLENGTH);
	while(read(sockfd, buf, MAXLENGTH)){
		if(buf[0] == LSWORK){
			read(sockfd, buf, MAXLENGTH);
			printf("%s\n",buf);
			read(sockfd, buf, MAXLENGTH);
			printf("%s\n",buf);
		}
		else
			printf("done!\n");
	}

	close(sockfd);*/
	//shutdown(sockfd, SHUT_WR);
		
	
	return 0;
}

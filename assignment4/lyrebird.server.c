#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <time.h>
#include <poll.h>

#define MAXCONNECTION 10000
#define MAXLENGTH 1200
#define LSWORK 0x0001
#define LSDONE 0x0002
#define LCSUCC 0x0004
#define LCFAIL 0x0008
#define LCREADY 0x0010

char *ipaddress;
char *buf;
char *encfile, *decfile;
FILE *fcfg, *flog;
struct pollfd fds[MAXCONNECTION];
int nfds;

char *getcurtime(){
	char *curtime;
	time_t ctm;
	time(&ctm);
	curtime = ctime(&ctm);
	curtime[strlen(curtime)-1] = 0;//eliminate '\n' at the end of string
	return curtime;
}

void getIPaddress(char *buf){
	struct ifaddrs *ifaddr, *ifa;
	void *tmpaddr;

	ifaddr = NULL;
	ifa = NULL;
	tmpaddr = NULL;

	getifaddrs(&ifaddr);
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
		if(!ifa->ifa_addr)
			continue;
		if(ifa->ifa_addr->sa_family == AF_INET) {
			tmpaddr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			if(!(IFF_LOOPBACK & ifa->ifa_flags)){
				inet_ntop(AF_INET, tmpaddr, buf, INET_ADDRSTRLEN);
				return;
			}
		}
	}
}

void terminate(int exitv){
	int i;
	for(i = 0; i < nfds; ++i) 
		close(fds[i].fd);
	free(buf);
	free(encfile);
	free(decfile);
	free(fcfg);
	free(flog);
	exit(exitv);
}

int main(int argc, char **argv){
	int i,j;
	int status;
	int cur_nfds,addrs;
	int on;
	int sockfd, csockfd;
	int addrindex[MAXCONNECTION];
	int taskover;
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr[MAXCONNECTION];
	//pfd *pfds, *npfd, *ptr;
	int retv;
	socklen_t len;

	if(argc!=3){
		printf("usage %s <config_file> <log_file>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	if(access(argv[1], F_OK) == -1) {
		printf("[%s] lyrebird.server PID %d cound not find file %s.\n", getcurtime(), getpid(), argv[1]);
		exit(EXIT_FAILURE);
	}
	fcfg = fopen(argv[1], "r+");
	if(fcfg == NULL){
		printf("[%s] lyrebird.server PID %d cound not open file %s.\n", getcurtime(), getpid(), argv[1]);
		exit(EXIT_FAILURE);
	}
	flog = fopen(argv[2], "w+");
	if(flog == NULL){
		printf("[%s] lyrebird.server PID %d cound not open file %s.\n", getcurtime(), getpid(), argv[2]);
		exit(EXIT_FAILURE);
	}

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	encfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	decfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	//ips = (char **)malloc(sizeof(char)*MAXLENGTH);
	//pfds = (pfd *)malloc(sizeof(pfd));
	memset(fds,0,sizeof(fds));

	taskover = 0;
	nfds = 0;
	getIPaddress(ipaddress);
	//printf("id: %s\n",ipaddress);
	len = sizeof(addr);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("[%s] lyrebird.server PID %d failed to call socket.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	addrindex[0] = 0;
	addrs = 1;
	on = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0 ){
		printf("[%s] lyrebird.server PID %d failed to call setsockopt.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	myaddr[0].sin_family = AF_INET;
	myaddr[0].sin_addr.s_addr = inet_addr(ipaddress);
	myaddr[0].sin_port = 0;

	if(bind(sockfd, (struct sockaddr *)&myaddr[0], sizeof(myaddr[0])) < 0){
		printf("[%s] lyrebird.server PID %d failed to call bind.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	if(getsockname(sockfd, (struct sockaddr *)&myaddr[0], &len) < 0) {
		printf("[%s] lyrebird.server PID %d failed to call getsockname.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	printf("[%s] lyrebird.server: PID %d on host %s, port %d\n",getcurtime(), getpid(), inet_ntoa(myaddr[0].sin_addr), ntohs(myaddr[0].sin_port));

	if(listen(sockfd, MAXCONNECTION) < 0){
		printf("[%s] lyrebird.server PID %d failed to call listen.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	/*fds->next = NULL;
	fds->fds.fd = sockfd;
	fds->fds.events = POLLIN;*/
	do{
		//printf("calling...\n");
		retv = poll(fds, nfds, -1);
		//printf("calling done...\n");
		if(retv < 0){
			printf("[%s] lyrebird.server PID %d failed to call poll.\n", getcurtime(), getpid());
			terminate(EXIT_FAILURE);
		}

		cur_nfds = nfds;
		for(i = 0; i < cur_nfds; ++i) {
			if(fds[i].revents == 0) 
				continue;
			//printf("socket: %d %d\n",fds[i].fd, fds[i].revents);
			/*if(fds[i].revents != POLLIN) {
				printf("[%s] ...\n",getcurtime());
				for(j = i; j < cur_nfds-1; ++j)
					fds[j] = fds[j+1];
				--nfds;
				--cur_nfds;
				--i;
				printf("disconnect!\n");
				for(j = 1; j < nfds; ++j)
					printf("%d ",fds[i].fd);
				printf("\n");
				continue;
			}*/

			/*
			a new request for connection
			*/
			if(fds[i].fd == sockfd) { 
				csockfd = accept(sockfd, (struct sockaddr *)&myaddr[addrs], &len);
				if(csockfd < 0){
					printf("[%s] lyrebird.server PID %d failed to call accept.\n", getcurtime(), getpid());
					terminate(EXIT_FAILURE);
				}
				fds[nfds].fd = csockfd;
				fds[nfds].events = POLLIN;
				addrindex[nfds] = addrs;
				++nfds;
				++addrs;
				//printf("connect!\n");
				//for(j = 1; j < nfds; ++j)
					//printf("%d ",fds[j].fd);
				//printf("\n");
				//fprintf(flog, "[%s] client with ip %s and port %d conneted.\n", getcurtime(), inet_ntoa(myaddr[addrindex[nfds-1]].sin_addr), ntohs(myaddr[addrindex[nfds-1]].sin_port));
				fprintf(flog, "[%s] Successfully connected to lyrebird client %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[nfds-1]].sin_addr));
			}
			/*
			a new message from old connection
			*/
			else{
				retv = read(fds[i].fd, buf, MAXLENGTH);
				//peer closed socket
				if(retv == 0) {
					close(fds[i].fd);
					if(taskover == 0)
						//printf("[%s] The lyrebird client %s has disconnected unexpectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
						fprintf(flog, "[%s] The lyrebird client %s has disconnected unexpectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
					else
						fprintf(flog, "[%s] The lyrebird client %s has disconnected expectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
						//printf("[%s] The lyrebird client %s has disconnected expectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
					for(j = i; j < cur_nfds-1; ++j) {
						fds[j] = fds[j+1];
						addrindex[j] = addrindex[j+1];
					}
					--nfds;
					--cur_nfds;
					--i;
					//printf("disconnect!\n");
					for(j = 1; j < nfds; ++j)
						printf("%d ",fds[i].fd);
					printf("\n");
					continue;
				}
				//printf("said %s.\n",buf);
				if(buf[0] == LCREADY) {
					//printf("ready!\n");
					if(EOF == fscanf(fcfg, "%s", encfile)) {
						taskover = 1;
						//printf("end\n");
						status = LSDONE;
						write(fds[i].fd, &status, MAXLENGTH);
						continue;
					}
					status = LSWORK;
					write(fds[i].fd, &status, MAXLENGTH);
					write(fds[i].fd, encfile, MAXLENGTH);
					fscanf(fcfg, "%s", decfile);
					printf("%s %s\n", encfile, decfile);
					write(fds[i].fd, decfile, MAXLENGTH);
					//printf("[%s] The lyrebird client %s has been given the task of decrypting %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, encfile);
					fprintf(flog, "[%s] The lyrebird client %s has been given the task of decrypting %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, encfile);
				}
				else if(buf[0] == LCFAIL) {
					//printf("fail!\n");
					read(fds[i].fd, buf, MAXLENGTH);
					//printf("[%s] The lyrebird client %s has encountered an error: %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
					fprintf(flog, "[%s] The lyrebird client %s has encountered an error: %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
				}
				else if(buf[0] == LCSUCC) {
					//printf("success!\n");
					read(fds[i].fd, buf, MAXLENGTH);
					fprintf(flog, "[%s] The lyrebird client %s has successfully decrypted %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
					//printf("[%s] The lyrebird client %s has successfully decrypted %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
				}
			}
		}
	} while(nfds>1);
	printf("[%s] lyrebird server: PID %d completed its tasks and is exiting successfully.\n", getcurtime(), getpid());

	terminate(EXIT_SUCCESS);
}

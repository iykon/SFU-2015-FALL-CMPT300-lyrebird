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

#define MAXCONNECTION 100
#define MAXLENGTH 1200
#define LSWORK 0x0001
#define LSDONE 0x0002
#define LCSUCC 0x0004
#define LCFAIL 0x0008
#define LCREADY 0x0010
/*
typedef struct pollfds{
	struct pollfd fds;
	struct pollfds *next;
} pfd;
*/
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

int main(int argc, char **argv){
	FILE *fcfg, *flog;
	int i,j;
	int status;
	int nfds,cur_nfds,addrs;
	int count, on;
	int sockfd, csockfd;
	int sockfds[MAXCONNECTION];
	int addrindex[MAXCONNECTION];
	int timeout;
	int taskover;
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr[MAXCONNECTION];
	struct pollfd fds[MAXCONNECTION];
	//pfd *pfds, *npfd, *ptr;
	int retv;
	char *ipaddress;
	char *buf, *rbuf;
	char *encfile, *decfile;
	char **ips;
	socklen_t len;

	if(argc!=3){
		printf("usage %s <config_file> <log_file>\n",argv[0]);
		exit(1);
	}

	if(access(argv[1], F_OK) == -1) {
		printf("[%s] ...\n",getcurtime());
		exit(1);
	}
	fcfg = fopen(argv[1], "r+");
	if(fcfg == NULL){
		printf("[%s] ...\n", getcurtime());
		exit(1);
	}
	flog = fopen(argv[2], "w+");
	if(flog == NULL){
		printf("[%s] ...\n",getcurtime());
		exit(1);
	}

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	rbuf = (char *)malloc(sizeof(char)*MAXLENGTH);
	encfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	decfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	//ips = (char **)malloc(sizeof(char)*MAXLENGTH);
	//pfds = (pfd *)malloc(sizeof(pfd));
	memset(fds,0,sizeof(fds));

	taskover = 0;
	getIPaddress(ipaddress);
	//printf("id: %s\n",ipaddress);
	len = sizeof(addr);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket");
		exit(1);
	}

	on = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0 ){
		perror("setsockopt");
		exit(1);
	}
	myaddr[0].sin_family = AF_INET;
	myaddr[0].sin_addr.s_addr = inet_addr(ipaddress);
	myaddr[0].sin_port = 0;

	if(bind(sockfd, (struct sockaddr *)&myaddr[0], sizeof(myaddr[0])) < 0){
		perror("bind");
		exit(1);
	}
	getsockname(sockfd, (struct sockaddr *)&myaddr[0], &len);
	printf("[%s] lyrebird.server: PID %d on host %s, port %d\n",getcurtime(), getpid(), inet_ntoa(myaddr[0].sin_addr), ntohs(myaddr[0].sin_port));

	if(listen(sockfd, MAXCONNECTION) < 0){
		perror("listen");
		exit(1);
	}
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	addrindex[0] = 0;
	addrs = 1;
	/*fds->next = NULL;
	fds->fds.fd = sockfd;
	fds->fds.events = POLLIN;*/
	do{
		retv = poll(fds, nfds, -1);
		if(retv < 0){
			perror("poll");
			exit(1);
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
					perror("accept");
					exit(1);
				}
				fds[nfds].fd = csockfd;
				fds[nfds].events = POLLIN;
				addrindex[nfds] = addrs;
				++nfds;
				++addrs;
				printf("connect!\n");
				for(j = 1; j < nfds; ++j)
					printf("%d ",fds[j].fd);
				printf("\n");
				//fprintf(flog, "[%s] client with ip %s and port %d conneted.\n", getcurtime(), inet_ntoa(myaddr[addrindex[nfds-1]].sin_addr), ntohs(myaddr[addrindex[nfds-1]].sin_port));
				printf("[%s] client with ip %s and port %d conneted.\n", getcurtime(), inet_ntoa(myaddr[addrindex[nfds-1]].sin_addr), ntohs(myaddr[addrindex[nfds-1]].sin_port));
			}
			/*
			a new message from old connection
			*/
			else{
				retv = read(fds[i].fd, buf, MAXLENGTH);
				//peer closed socket
				if(retv == 0) {
					if(taskover == 0)
						printf("[%s] The lyrebird client %s has disconnected unexpectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
					else
						printf("[%s] The lyrebird client %s has disconnected expectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/);
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
					write(fds[i].fd, decfile, MAXLENGTH);
					printf("[%s] The lyrebird client %s has been given the task of decrypting %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, encfile);
				}
				else if(buf[0] == LCFAIL) {
					//printf("fail!\n");
					read(fds[i].fd, buf, MAXLENGTH);
					printf("[%s] The lyrebird client %s has encountered an error: %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
				}
				else if(buf[0] == LCSUCC) {
					//printf("success!\n");
					read(fds[i].fd, buf, MAXLENGTH);
					printf("[%s] The lyrebird client %s has successfully decrypted %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
				}
				else{
					printf("others\n");
				}
			}
		}
	}while(nfds>1);

	printf("[%s] lyrebird server: PID %d completed its tasks and is exiting successfully.\n", getcurtime(), getpid());


	return 0;
}

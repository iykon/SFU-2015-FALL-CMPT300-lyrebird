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
#define SUCCESS "!success"
#define FAIL "!fail"
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
	int nfds,cur_nfds;
	int count, on;
	int sockfd, csockfd;
	int sockfds[MAXCONNECTION];
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr;
	struct pollfd fds[MAXCONNECTION];
	//pfd *pfds, *npfd, *ptr;
	int retv;
	char *ipaddress;
	char *buf, *rbuf;
	char *encfile, *decfile;
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
	//pfds = (pfd *)malloc(sizeof(pfd));
	memset(fds,0,sizeof(fds));

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
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(ipaddress);
	myaddr.sin_port = 0;

	if(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0){
		perror("bind");
		exit(1);
	}
	getsockname(sockfd, (struct sockaddr *)&myaddr, &len);
	printf("[%s] lyrebird.server: PID %d on host %s, port %d\n",getcurtime(), getpid(), inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port));

	if(listen(sockfd, MAXCONNECTION) < 0){
		perror("listen");
		exit(1);
	}
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	/*fds->next = NULL;
	fds->fds.fd = sockfd;
	fds->fds.events = POLLIN;*/
	while(1){
		retv = poll(fds, nfds, -1);
		if(retv < 0){
			perror("poll");
			exit(1);
		}

		cur_nfds = nfds;
		for(i = 0; i < cur_nfds; ++i) {
			if(fds[i].revents == 0) 
				continue;
			if(fds[i].revents != POLLIN) {
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
			}
			if(fds[i].fd == sockfd) {  //new connection
				csockfd = accept(sockfd, (struct sockaddr *)&myaddr, &len);
				if(csockfd < 0){
					perror("accept");
					exit(1);
				}
				fds[nfds].fd = csockfd;
				fds[nfds].events = POLLIN;
				++nfds;
				printf("connect!\n");
				for(j = 1; j < nfds; ++j)
					printf("%d ",fds[j].fd);
				printf("\n");
				//fprintf(flog, "[%s] client with ip %s and port %d conneted.\n", getcurtime(), inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port));
			}
			else{   // new message
				read(fds[i].fd, buf, MAXLENGTH);
				if(strcmp(buf, SUCCESS) == 0) {
					printf("success!\n");
					if(EOF == fscanf(fcfg, "%s", encfile)) {
						printf("end\n");
						exit(1);
					}
					write(fds[i].fd, encfile, MAXLENGTH);
					fscanf(fcfg, "%s", decfile);
					write(fds[i].fd, decfile, MAXLENGTH);
				}
				else if(strcmp(buf, FAIL) == 0) {
					printf("fail!\n");
				}
				else{
					printf("others\n");
				}
			}
		}
	}


	return 0;
}

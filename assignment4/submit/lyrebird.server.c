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
 * file name: lyrebird.server.c
 * description: This is the main source code of server.
 * 				This server first creates a socket and
 * 				binds it with the IP of the running
 * 				machine and a random availble port.
 * 				Then server accepts client connections 
 * 				through internet and sends or receives 
 * 				messages though sockets.
 * */
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
#include "memwatch.h"
#include "msgprotocol.h"

#define MAXCONNECTION 10000 // maximum number of client connections
#define MAXLENGTH 1200 // maximum length of file name and tweets

char *ipaddress; // ip of running machine
char *buf;	// buffer for sending and receiving message
char *encfile, *decfile; // file name
FILE *fcfg, *flog; // file pointer
struct pollfd fds[MAXCONNECTION]; // objects for function poll()
int nfds; // number of objects in poll()

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
 * function name: getIPaddress
 * description: Get the IP address of running machine
 * arguments: char *buf: store the ip in this buffer
 * return value: none
 * */
void getIPaddress(char *buf){
	struct ifaddrs *ifaddr, *ifa;
	void *tmpaddr;

	ifaddr = NULL;
	ifa = NULL;
	tmpaddr = NULL;

	/* get a linked list of structures describing 
	 * the network interfaces of the local system
	 * */
	getifaddrs(&ifaddr);
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){ // scan all interfaces
		if(!ifa->ifa_addr)
			continue;
		if(ifa->ifa_addr->sa_family == AF_INET) { // get IPv4 only
			tmpaddr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			if(!(IFF_LOOPBACK & ifa->ifa_flags)){ // escape loopback
				inet_ntop(AF_INET, tmpaddr, buf, INET_ADDRSTRLEN);
				return;
			}
		}
	}
}

/*
 * function: terminate
 * description: Free memory and exit
 * arguments: int exitv: exit value of program
 * return value: none
 * */
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

/*
 * function: main
 * description: Create socket and accept internet connection
 * 				using first come first server, assign tasks
 * 				to clients through sockets. Also read clients
 * 				status from sockets. Write all necessary states
 * 				into log file. When all done, tell clients to
 * 				exit through sockets.
 * arguments: int argc: number of arguments for executing
 * 			  char **argv: the list of arguments
 * */
int main(int argc, char **argv){
	int i,j;
	int status; // current status telling client whether to decrypt or to exit
	int cur_nfds,addrs;
	int on;
	int sockfd, csockfd; //socket file descriptor
	int addrindex[MAXCONNECTION]; // all clients' ip addresses
	int taskover; // indicating whether all tasks are assigned
	int retv; // return value of a function call
	socklen_t len; // struct sockaddr length
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr[MAXCONNECTION]; // client connection information

	if(argc!=3){ // need only 3 arguments
		printf("usage %s <config_file> <log_file>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	if(access(argv[1], F_OK) == -1) { // file not found
		printf("[%s] lyrebird.server PID %d cound not find file %s.\n", getcurtime(), getpid(), argv[1]);
		exit(EXIT_FAILURE);
	}
	fcfg = fopen(argv[1], "r+");
	if(fcfg == NULL){ // file open failed
		printf("[%s] lyrebird.server PID %d cound not open file %s.\n", getcurtime(), getpid(), argv[1]);
		exit(EXIT_FAILURE);
	}
	flog = fopen(argv[2], "w+");
	if(flog == NULL){ // file open failed
		printf("[%s] lyrebird.server PID %d cound not open file %s.\n", getcurtime(), getpid(), argv[2]);
		exit(EXIT_FAILURE);
	}

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	encfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	decfile = (char *)malloc(sizeof(char)*MAXLENGTH);
	memset(fds,0,sizeof(fds));

	//initialization
	taskover = 0;
	nfds = 0;
	getIPaddress(ipaddress);
	len = sizeof(addr);
	
	// create socket for accepting conection
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("[%s] lyrebird.server PID %d failed to call socket.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	//add socket into the array
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	addrindex[0] = 0;
	addrs = 1;
	on = 1;
	// set socket option so that can be used below
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0 ){
		printf("[%s] lyrebird.server PID %d failed to call setsockopt.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	myaddr[0].sin_family = AF_INET; // IPv4 specified
	myaddr[0].sin_addr.s_addr = inet_addr(ipaddress); // IP address specified
	myaddr[0].sin_port = 0; // port number 0, let OS assign a random availble port

	// bind socket with IP and port
	if(bind(sockfd, (struct sockaddr *)&myaddr[0], sizeof(myaddr[0])) < 0){
		printf("[%s] lyrebird.server PID %d failed to call bind.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	// get socket information, here only IP address and port number needed
	if(getsockname(sockfd, (struct sockaddr *)&myaddr[0], &len) < 0) {
		printf("[%s] lyrebird.server PID %d failed to call getsockname.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}
	printf("[%s] lyrebird.server: PID %d on host %s, port %d\n",getcurtime(), getpid(), inet_ntoa(myaddr[0].sin_addr), ntohs(myaddr[0].sin_port));

	// call listen so that sockfd can handle connection requests
	if(listen(sockfd, MAXCONNECTION) < 0){
		printf("[%s] lyrebird.server PID %d failed to call listen.\n", getcurtime(), getpid());
		terminate(EXIT_FAILURE);
	}

	do{
		// wait for any message either a new connection request or a disconnection of a new message
		retv = poll(fds, nfds, -1);
		if(retv < 0){
			printf("[%s] lyrebird.server PID %d failed to call poll.\n", getcurtime(), getpid());
			terminate(EXIT_FAILURE);
		}

		cur_nfds = nfds;
		for(i = 0; i < cur_nfds; ++i) {
			if(fds[i].revents == 0) // socket with no message
				continue;

			// a new request for connection
			if(fds[i].fd == sockfd) { 
				csockfd = accept(sockfd, (struct sockaddr *)&myaddr[addrs], &len);
				if(csockfd < 0){
					printf("[%s] lyrebird.server PID %d failed to call accept.\n", getcurtime(), getpid());
					terminate(EXIT_FAILURE);
				}
				// add new client into the array
				fds[nfds].fd = csockfd;
				fds[nfds].events = POLLIN;
				addrindex[nfds] = addrs;
				++nfds;
				++addrs;
				fprintf(flog, "[%s] Successfully connected to lyrebird client %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[nfds-1]].sin_addr));
			}
			/* 
			 * a new message from old connection or a disconnection
			 * if read() return a length of 0, it means socket is closed,
			 * which indicates client disconnected
			 * if read() return a length of a positive number,
			 * it means server gets a new message
			 * */
			else{
				retv = read(fds[i].fd, buf, MAXLENGTH);
				if(retv == 0) { // client disconnets
					close(fds[i].fd);
					if(taskover == 0)
						fprintf(flog, "[%s] The lyrebird client %s has disconnected unexpectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr));
					else
						fprintf(flog, "[%s] The lyrebird client %s has disconnected expectedly.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr));
					// delete the client from the array
					for(j = i; j < cur_nfds-1; ++j) {
						fds[j] = fds[j+1];
						addrindex[j] = addrindex[j+1];
					}
					--nfds;
					--cur_nfds;
					--i;
					continue;
				}
				if(buf[0] == LCREADY) { // client is ready for a task
					if(EOF == fscanf(fcfg, "%s", encfile)) { // all tasks distributed
						taskover = 1;
						//tell clients to exit
						status = LSDONE;
						write(fds[i].fd, &status, MAXLENGTH);
						continue;
					}
					//tell client to work
					status = LSWORK;
					write(fds[i].fd, &status, MAXLENGTH);
					write(fds[i].fd, encfile, MAXLENGTH);
					fscanf(fcfg, "%s", decfile);
					printf("%s %s\n", encfile, decfile);
					write(fds[i].fd, decfile, MAXLENGTH);
					fprintf(flog, "[%s] The lyrebird client %s has been given the task of decrypting %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr), encfile);
				}
				else if(buf[0] == LCFAIL) { // client failed due to some error
					read(fds[i].fd, buf, MAXLENGTH); // read error message
					fprintf(flog, "[%s] The lyrebird client %s has encountered an error: %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr), buf);
				}
				else if(buf[0] == LCSUCC) { // client successfully decrypted a file
					read(fds[i].fd, buf, MAXLENGTH); // read the file name
					fprintf(flog, "[%s] The lyrebird client %s has successfully decrypted %s.\n", getcurtime(), inet_ntoa(myaddr[addrindex[i]].sin_addr)/*ntohs(myaddr[addrindex[i]].sin_port)*/, buf);
				}
			}
		}
	} while(nfds>1);
	printf("[%s] lyrebird server: PID %d completed its tasks and is exiting successfully.\n", getcurtime(), getpid());

	terminate(EXIT_SUCCESS);
}

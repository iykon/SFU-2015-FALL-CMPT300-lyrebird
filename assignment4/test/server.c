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
#include <poll.h>

#define PORT 2333
#define MAXLENGTH 1200
#define MAXCONNECTIONS 10

void getIPaddress(char *addressBuffer){
	struct ifaddrs *ifAddrStruct=NULL;
    struct ifaddrs *ifa=NULL;
    void *tmpAddrPtr=NULL;
    //char *addressBuffer;

	//addressBuffer = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            //printf("%s IPv4 Address %s with flags %d and LOOPBACK: %d\n", ifa->ifa_name, addressBuffer,ifa->ifa_flags,IFF_LOOPBACK&ifa->ifa_flags); 
			if(!(IFF_LOOPBACK & ifa->ifa_flags)){
            	inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				return;// addressBuffer;
			}
        }/* else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("%s IPv6 Address %s\n", ifa->ifa_name, addressBuffer); 
        } */
    }
}

int main(int argc, char **argv){
	int i,j;
	int nfds;
	int count, on = 1;
	int sockfd,csockfd;
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr;
	struct pollfd fds[MAXCONNECTIONS];
	int retv;
	char *ipaddress;
	char *buf;

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	memset(fds, 0, sizeof(fds));
	//fds = (struct pollfd *)malloc(sizeof(struct pollfd)*MAXCONNECTIONS);
	//memset(addr, 0, sizeof addr);

	getIPaddress(ipaddress);
	printf("in main got ip address is %s.\n",ipaddress);


	socklen_t len = sizeof(addr);
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1){
		perror("socket");
		exit(1);
	}
	setsockopt(sockfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr(ipaddress);
    myaddr.sin_port = 0;
	
	if(bind(sockfd,  (struct sockaddr *)&myaddr, sizeof myaddr)==-1){
		perror("bind");
		exit(1);
	}
	getsockname(sockfd, (struct sockaddr *)&myaddr, /*(socklen_t *)(sizeof myaddr)*/&len); 
	printf("ip address: %s.\nport is: %d.\n",inet_ntoa(myaddr.sin_addr),ntohs(myaddr.sin_port));
	count = 0;
	listen(sockfd,MAXCONNECTIONS);
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	printf("calling poll()...\n");
	retv = poll(fds,nfds,-1);
	if(retv < 0){
		perror("poll");
		exit(1);
	}
	printf("calling poll() done.\n");
	//while(1){
	csockfd = accept(sockfd, (struct sockaddr *)&myaddr, &len);
	if(csockfd < 0){
		perror("accept");
		exit(1);
	}
	++count;
	printf("Little bitcha, I am accepted!\n");
	//getsockname(csockfd, (struct sockaddr *)&myaddr, /*(socklen_t *)(sizeof myaddr)*/&len); 
	printf("ip address: %s.\nport is: %d.\n",inet_ntoa(myaddr.sin_addr),ntohs(myaddr.sin_port));
	scanf("%s",buf);
	write(csockfd, buf, MAXLENGTH);
	close(csockfd);
	//}

	close(sockfd);
	
	printf("number: %d\n",count);
	return 0;
}

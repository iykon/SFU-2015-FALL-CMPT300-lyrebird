#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#define PORT 2333

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
	int sockfd;
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr;
	int retv;
	char *ipaddress;

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	//memset(addr, 0, sizeof addr);

	getIPaddress(ipaddress);
	printf("in main got ip address is %s.\n",ipaddress);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((retv = getaddrinfo(ipaddress, 0, &hints, &serverinfo)) != 0 ){
		printf("getaddrinfo: %s\n", gai_strerror(retv));
		exit(1);
	}
	for(p =  serverinfo; p != NULL; p = p->ai_next){
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("socket");
			continue;
		}
		//printf("%d %d %d.\n",p->ai_family, p->ai_socktype, p->ai_protocol);
		//if(getsockname(sockfd,p->ai_addr, p->ai_addrlen)==0)
			//printf("port number: %d.\n",);
		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("bind");
			continue;
		}
		break;
	}
	if(p == NULL){
		printf("failed to connect\n");
		exit(2);
	}
	
	//printf("IP address: %s and port number: %d.\n",p->ai_addr->sa_data,PORTNUMBER);
	
	//freeaddrinfo(serverinfo);
	/*if((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1){
		perror("socket");
		exit(1);
	}
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = 0;     // short, network byte order
	myaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr("10.12.110.57");
	memset(myaddr.sin_zero, '\0', sizeof myaddr.sin_zero);
	
	if(bind(sockfd,  (struct sockaddr *)&myaddr, sizeof myaddr)==-1){
		perror("bind");
		exit(1);
	}
	*/
	listen(sockfd,100);
	accept(sockfd,NULL,NULL);
	getsockname(sockfd, (struct sockaddr *)&myaddr, (socklen_t *)(sizeof myaddr)); 
	printf("ip address: %s.\nport is: %d.\n",inet_ntoa(myaddr.sin_addr),(int) ntohs(myaddr.sin_port));
	return 0;
}

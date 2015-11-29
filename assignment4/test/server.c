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
#include <time.h>

#define PORT 2333
#define MAXLENGTH 1200
#define MAXCONNECTIONS 10

char *getcurtime(){
	char *curtime;
	time_t ctm;
	time(&ctm);
	curtime = ctime(&ctm);
	curtime[strlen(curtime)-1] = 0;//eliminate '\n' at the end of string
	return curtime;
}


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
            	inet_ntop(AF_INET,tmpAddrPtr,addressBuffer, INET_ADDRSTRLEN);
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
	int nfds,cur_nfds;
	int count, on = 1;
	int sockfd,csockfd;
	int sockfds[1000];
	struct addrinfo hints, *serverinfo, *p;
	struct sockaddr addr;
	struct sockaddr_in myaddr;
	struct pollfd fds[MAXCONNECTIONS];
	int retv;
	char *ipaddress;
	char *buf,*rbuf;

	ipaddress = (char *)malloc(sizeof(char)*INET_ADDRSTRLEN);
	buf = (char *)malloc(sizeof(char)*MAXLENGTH);
	rbuf = (char *)malloc(sizeof(char)*MAXLENGTH);
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
	//printf("%s\n",strcat("ha","nono"));
	count = 0;
	listen(sockfd,MAXCONNECTIONS);
	nfds = 1;
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	while(count<10){
		//printf("[%s] calling poll()...\n",getcurtime());
		retv = poll(fds,nfds,-1);
		if(retv < 0){
			perror("poll");
			exit(1);
		}
		//printf("[%s] calling poll() done.\n",getcurtime());
		//while(1){
		cur_nfds = nfds;
		//printf("[%s] current nfsd: %d\n",getcurtime(),nfds);
		for(i = 0; i < cur_nfds; ++i){
			if(fds[i].revents == 0)
				continue;
			if(fds[i].revents != POLLIN){
				//printf("poll error!\n");
				//exit(1);
				printf("%d is off line.\n", fds[i].fd);
				exit(1);
			}
			if(fds[i].fd == sockfd){
				csockfd = accept(sockfd, (struct sockaddr *)&myaddr, &len);
				if(csockfd < 0){
					perror("accept");
					exit(1);
				}
				sockfds[count++] = csockfd;
				//printf("[%s] Little bitcha, I am accepted!\n",getcurtime());
				//getsockname(csockfd, (struct sockaddr *)&myaddr, /*(socklen_t *)(sizeof myaddr)*/&len); 
				printf("[%s] ip address: %s.\nport is: %d.\n",getcurtime(),inet_ntoa(myaddr.sin_addr),ntohs(myaddr.sin_port));
				printf("[%s] current connected: %d\n", getcurtime(), count);
				//scanf("%s",buf);
				buf = "congratulations for connecting!";
				//write(csockfd, buf, MAXLENGTH);
				write(csockfd,buf,MAXLENGTH);
				fds[nfds].fd = csockfd;
				fds[nfds].events = POLLIN;
				++nfds;
				printf("?\n");
			}
			else{
				//printf("there is a message.\n");
				read(fds[i].fd, rbuf, MAXLENGTH);
				printf("socked id: %d\n",fds[i].fd);
				//printf("message got.\n");
				buf = "Someone said: ";
				//strcat(buf, rbuf);
				//printf("sending messages\n");
				//for(i = 0; i < nfds; ++i)
					//write(fds[i].fd,rbuf,MAXLENGTH);
				printf("[%s] messages get: %s.\n",getcurtime(),rbuf);
			}
		}
	}
	//close(csockfd);
	//}

	close(sockfd);
	
	printf("number: %d\n",count);
	return 0;
}

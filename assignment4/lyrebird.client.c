#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXLENGTH 1200

int main(int argc, char **argv){
	char c;
	int sockfd;
	int status;
	int retv;
	struct addrinfo hints;
	struct addrinfo *serverinfo;
	struct sockaddr_in addr;
	char *buf;

	if(argc!=3){
		printf("usage: %s <server IP address> <port number>\n",argv[0]);
		exit(1);
	}

	buf = (char *)malloc(sizeof(char)*MAXLENGTH);

	/*memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if((status = getaddrinfo(argv[1],argv[2], &hints, &serverinfo)) != 0){
		printf("getaddrinfo: %s\n", gai_strerror(status));
		exit(2);
	}*/
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
	printf("connect!\n");
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
	write(sockfd, "!success", MAXLENGTH);
	while(read(sockfd, buf, MAXLENGTH)){
		printf("%s\n",buf);
	}

	close(sockfd);
	return 0;
}

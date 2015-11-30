#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#define MAXLENGTH 1200

const char success[100] = "!success";
const char fail[100] = "!fail";

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

	buf = (char *)malloc(sizeof(char)*(MAXLENGTH+2));

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
	while(1){
		printf("?\n");
		scanf("%s",buf);
		retv = send(sockfd, (char *)fail, MAXLENGTH, 0);
		if(retv < 0){
			printf("%d\n",errno);
			switch (errno){
				case EAGAIN:
					printf("1\n");
					break;
				case EBADF:
					printf("2\n");
					break;
				case EFAULT:
					printf("3\n");
					break;
				case EINTR:
					printf("4\n");
					break;
				case EINVAL:
					printf("5\n");
					break;
				case EIO:
					printf("6\n");
					break;
				case EISDIR:
					printf("7\n");
					break;
				default:break;
}
		exit(1);
}
		printf("write into socket with return value %d\n",retv);
	}

	close(sockfd);
	return 0;
}

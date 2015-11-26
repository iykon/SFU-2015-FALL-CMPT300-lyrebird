#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    addr.sin_port = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket()");
        return -1;
    }
    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        perror("bind()");
        return -1;
    }
    if (getsockname(sock, (struct sockaddr*) &addr, &len) != 0) {
        perror("getsockname()");
        return -1;
    }
    printf("%d\n", addr.sin_port);
	close(sock);
    return 0;
}

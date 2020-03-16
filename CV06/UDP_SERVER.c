#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include<sys/types.h>
#include<sys/socket.h>

#include<netinet/ip.h>
#include<netinet/in.h>

#include<arpa/inet.h>

#define PORT 1234
#define IP_ADDR "127.0.0.1"

int main() {

    int sock;
    struct sockaddr_in addr;

    socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1) {
        perror("Socket error: ");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if(inet_aton(IP_ADDR, &addr.sin_addr) == 0) {
        perror("inet_aton: ");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(bind(sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1){
    perror("bind");
    close(sock);
    exit(EXIT_FAILURE);
    }

    for(;;) {
        char buf[1000];
        memset(buf, '\0', sizeof(buf));
        int addr_len = sizeof(addr);
        recvfrom(sock, buf, 1000, 0, (struct sockaddr *) &addr, &addr_len);
        char *ip = inet_ntoa(addr.sin_addr);

        printf("Message from: %s:%d text: %s",
            ip,
            ntohs(addr.sin_port),
            buf);
    }

    return EXIT_SUCCESS;
}
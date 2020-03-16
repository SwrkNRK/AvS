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
        printf("ERROR: inet_aton: ");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        perror("BIND:");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
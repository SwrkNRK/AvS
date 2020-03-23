#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1234
#define IP_ADDR "10.123.123.255"

int main()
{
	int sock;
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	if(inet_aton(IP_ADDR, &addr.sin_addr) == 0)
	{
		printf("ERROR: inet_aton");
		close(sock);
		exit(EXIT_FAILURE);
	}
    
    int allowBroadcast = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &allowBroadcast, sizeof(allowBroadcast)) == -1){
        perror("setsockopt");
        close(sock);
        exit(EXIT_FAILURE);
    }


	char buf[1000];
	memset(buf, '\0', 1000);
	int addr_len = sizeof(addr);
	fgets(buf, 1000, stdin);


	sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &addr, addr_len);




	return EXIT_SUCCESS;
}

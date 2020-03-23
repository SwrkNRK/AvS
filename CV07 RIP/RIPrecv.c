#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#define PORT 520
#define IP "10.123.123.242"
#define IF "tap0"

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
   // addr.sin_addr.s_addr = INADDR_ANY;  // pocuvaj na vsetkych rozhraniach
	inet_aton(IP, &addr.sin_addr);			//pocuvaj na rozhrani tap0

	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		close(sock);
		exit(EXIT_FAILURE);
	}
    
    struct ip_mreqn multicast;
	memset(&multicast, 0, sizeof(multicast));
    //multicast.imr_address.s_addr = INADDR_ANY;  //IP hociktooreho lokalneho rozhrania
    // multicast.imr_ifindex = 0;                  //vsetko rozhranias
	inet_aton(IP, &multicast.imp_address);			//pocuvaj na rozhrani tap0
	multicast.imp_ifindex = if_nametoindex(IF);
    if(inet_aton("224.0.0.9", &multicast.imr_multiaddr) == 0) {
        printf("inet_ATON");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast, sizeof(struct ip_mreqn)) == -1){
        perror("setsockopt");
        close(sock);
        exit(EXIT_FAILURE);
    }



	for(;;)
	{
		char buf[1000];

		memset(buf, '\0', 1000);
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//Na Sockety
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <arpa/inet.h>


#define EXIT_OK 0
#define EXIT_ERROR 1
#define ETH "eth0"
#define SMAC "08:00:27:fd:56:68"

struct ethHdr {
	char dstAddr[6];
	char srcAddr[6];
	uint16_t type;
	char payload[60];

}__attribute__((packed));


int main() {

	int sock;
	struct sockaddr_ll addr;
	struct ethHdr eth;

	sock = socket(AF_PACKET, SOCK_RAW, 0);
	if(sock == -1) {perror("Socket: "); return EXIT_ERROR;}

	// init struct sockaddr_ll addr
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ALL);
	addr.sll_ifindex = if_nametoindex(ETH); // linux indexuje interfaci, nutne pouzit nametoindex
	//end of init struct sockaddr_ll addr

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {	// naviažem adresu k soketu, tj previažem soket s fyzickým rozhraním
		perror("BIND ERROR: "); return EXIT_ERROR;
	}

	//init struct ethHdr eth
	memset(&eth, 0, sizeof(eth));
	eth.srcAddr[0] = 0x08;
	eth.srcAddr[1] = 0x00;
	eth.srcAddr[2] = 0x27;
	eth.srcAddr[3] = 0xfd;
	eth.srcAddr[4] = 0x56;
	eth.srcAddr[5] = 0x68;

	memset(eth.dstAddr, 0xFF, sizeof(eth.dstAddr));

	eth.type = htons(0x1234);

	strcpy(eth.payload, "Hello world. R A M E C, I am World");

	// end of init struct ethHdr eth

	write(sock, &eth, sizeof(eth));
	close(sock);

	printf("Exit Success\n");
	printf("Exit Success\n");
	return EXIT_OK;
}

















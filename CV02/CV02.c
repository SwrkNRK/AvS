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
#include <netinet/in.h>


#define EXIT_OK 0
#define EXIT_ERROR 1

#define ETH "eth0"
#define SMAC "08:00:27:fd:56:68"
#define IP "192.168.137.195"
#define TARGET_IP "192.168.137.1"

#define HW_TYPE 1
#define PROTO_TYPE 0x0800
#define HW_LEN 6
#define PROTO_LEN 4
#define OP_REQUEST 1
#define OP_REPLY 2


struct ethHdr {
	char dstAddr[6];
	char srcAddr[6];
	uint16_t type;
	char payload[0];    //odkazovanie na strukturu ktora bude zacinat na adrese payload

}__attribute__((packed));

struct arpHdr {
    uint16_t hwType;
    uint16_t protoType;
    uint8_t hwLen;
    uint8_t protoLen;
    uint16_t opcode;
    char sendHwAddr[6];
    struct in_addr sendIP;
    char targetHwAddr[6];
    struct in_addr targetIP;

}__attribute__((packed));


int main() {

	int sock;
	struct sockaddr_ll addr;
	struct ethHdr eth;
    struct arpHdr arp;

	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if(sock == -1) {perror("Socket: "); return EXIT_ERROR;}

	// init struct sockaddr_ll addr
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ARP);
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

	eth.type = htons(ETHERTYPE_ARP);
	// end of init struct ethHdr eth

    //init struct arpHdr arp
    memset(&arp, 0, sizeof(arp));
    arp.hwType = htons(HW_TYPE);
    arp.protoType = htons(PROTO_TYPE);
    arp.hwLen = HW_LEN;
    arp.protoLen = PROTO_LEN;
    arp.opcode = htons(OP_REQUEST);
    memcpy(arp.sendHwAddr, eth.srcAddr, sizeof(eth.srcAddr));
    inet_aton(IP, &arp.sendIP);
    inet_aton(TARGET_IP, &arp.targetIP);
    //end of init struct arpHdr arp

    int bufLen = sizeof(eth) + sizeof(arp);
    char buf[bufLen];
    memcpy(buf, &eth,sizeof(eth));
    memcpy(buf+sizeof(eth), &arp, sizeof(arp));


	write(sock, buf, sizeof(buf));
    struct arpHdr *arp1;
    while(1) {
    memset(buf, 0, sizeof(buf));
    read(sock, buf, sizeof(buf));


    struct ethHdr *eth1;
    eth1 = (struct ethHdr *)buf;

    if(eth1->type != htons(ETHERTYPE_ARP)){ continue; }

        //struct arpHdr *arp1;
        arp1 = (struct arpHdr *) eth1->payload;
        
        if(arp1->hwType != htons(HW_TYPE)) { continue; }

        if(arp1->protoType != htons(PROTO_TYPE)) { continue; }

        if(arp1->opcode != htons(OP_REPLY)){ continue; }

        if(strcmp(inet_ntoa(arp1->sendIP), TARGET_IP) != 0){ continue; }

        break;
    }

    printf("MAC: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
            arp1->sendHwAddr[0],
            arp1->sendHwAddr[1],
            arp1->sendHwAddr[2],
            arp1->sendHwAddr[3],
            arp1->sendHwAddr[4],
            arp1->sendHwAddr[5]);

	close(sock);

	printf("Exit Success\n");
	printf("Exit Success\n");
	return EXIT_OK;
}














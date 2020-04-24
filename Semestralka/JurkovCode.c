#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/route.h>
#include <time.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include <string.h>
#include <asm/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>


#define		RIP_GROUP	"224.0.0.9"
#define		RIP_VERSION	(2)
#define		PORT		(520)
#define		MAXRIPENTRIES	(25)
#define		MSGLEN		(sizeof (struct RIPMessage) + MAXRIPENTRIES * sizeof (struct RIPNetEntry))

#define		IPTXTLEN	(4*3 + 3 + 1)

#define		RIP_M_RESP	(2)
#define		RIP_N_METRIC	(1)

#define		EXIT_ERROR	(1)
#define ETH "eth2"

struct RIPNetEntry
{
  unsigned short int AF;
  unsigned short int Tag;
  struct in_addr Net;
  struct in_addr Mask;
  struct in_addr NHop;
  unsigned int Metric;
} __attribute__ ((packed));

struct RIPMessage
{
  unsigned char Command;
  unsigned char Version;
  unsigned short int Unused;
  struct RIPNetEntry Entry[0];
} __attribute__ ((packed));

#pragma pack(2)
// Structure for sending the request
typedef struct
{
struct nlmsghdr nlMsgHdr;
struct rtmsg rtMsg;
char buf[1024];
}route_request;

// Structure for storing routes
struct RouteInfo
{
unsigned long dstAddr;
unsigned long mask;
unsigned long gateWay;
unsigned long flags;
unsigned long srcAddr;
unsigned char proto;
char ifName[IF_NAMESIZE];
};

struct RouteInfo route[25];
int pocetRouteZaznamov;

/*--------------------------------------------------------------
* To get the name of the interface provided the interface index
*--------------------------------------------------------------*/
int ifname(int if_index,char *ifName)
{
int fd,retVal = -1;
struct sockaddr_in *sin;
struct ifreq ifr;


fd = socket(AF_INET, SOCK_DGRAM, 0);
if(fd == -1)
{
perror("socket");
exit(1);
}

ifr.ifr_ifindex = if_index;

if(ioctl(fd, SIOCGIFNAME, &ifr, sizeof(ifr)))
{
perror("ioctl");
exit(1);
}

strcpy(ifName, ifr.ifr_name);
return if_index;
} 


void *
SenderThread (void *Arg)
{
  int Socket = *((int *) Arg);
  struct RIPMessage *RM = (struct RIPMessage *) malloc (MSGLEN);
  int BytesToSend;
  struct sockaddr_in DstAddr;
  struct timespec TimeOut;

  //EDIT
  char Network[IPTXTLEN] = "192.168.20.0";
	char Netmask[IPTXTLEN] = "255.255.255.0";
	char NextHop[IPTXTLEN] = "0.0.0.0";

  route[0].dstAddr = inet_addr(Network);
  route[0].gateWay = inet_addr(NextHop);
  route[0].mask = inet_addr(Netmask);

  if (RM == NULL)
    {
      perror ("SenderThread malloc");
      close (Socket);
      exit (EXIT_ERROR);
    }

  memset (RM, 0, MSGLEN);
  RM->Command = RIP_M_RESP;
  RM->Version = RIP_VERSION;

  memset (&DstAddr, 0, sizeof (DstAddr));
  DstAddr.sin_family = AF_INET;
  DstAddr.sin_port = htons (PORT);
  if (inet_aton (RIP_GROUP, &DstAddr.sin_addr) == 0)
    {
      fprintf (stderr, "Error: %s is not a valid IPv4 address.\n\n",
	       RIP_GROUP);
      exit (EXIT_ERROR);
    }

  for (;;)
    {
      struct RIPNetEntry *E;
      int ECount;

      E = RM->Entry;
      memset (RM->Entry, 0, MAXRIPENTRIES * sizeof (struct RIPNetEntry));
      BytesToSend = sizeof (struct RIPMessage);
      ECount = 0;

      TimeOut.tv_sec = 10;
      TimeOut.tv_nsec = 0;
      nanosleep (&TimeOut, NULL);

      for (int i = 0; i < 1; i++)                                       //POSIELANIE
	{
	  E->AF = htons (AF_INET);
	  //E->Net.s_addr = htonl ((10 << 24) + (102 << 16) + (i << 8));
    E->Net.s_addr = route[i].dstAddr;
	  E->Mask.s_addr = route[i].mask;
	  E->Metric = htonl (RIP_N_METRIC);
    E->NHop.s_addr = route[i].gateWay;
	  BytesToSend += sizeof (struct RIPNetEntry);
	  E++;
	  ECount++;

	  if (ECount == MAXRIPENTRIES)
	    {
	      if (sendto
		  (Socket, RM, BytesToSend, 0, (struct sockaddr *) &DstAddr,
		   sizeof (DstAddr)) == -1)
		{
		  perror ("SenderThread sendto");
		  close (Socket);
		  exit (EXIT_ERROR);
		}

	      TimeOut.tv_sec = 0;
	      TimeOut.tv_nsec = 10 * 1000 * 1000;
	      nanosleep (&TimeOut, NULL);

	      memset (RM->Entry, 0,
		      MAXRIPENTRIES * sizeof (struct RIPNetEntry));
	      BytesToSend = sizeof (struct RIPMessage);
	      ECount = 0;
	      E = RM->Entry;
	    }
	}

      if (ECount == 0)
	continue;

      if (sendto
	  (Socket, RM, BytesToSend, 0, (struct sockaddr *) &DstAddr,
	   sizeof (DstAddr)) == -1)
	{
	  perror ("SenderThread sendto");
	  close (Socket);
	  exit (EXIT_ERROR);
	}
    }
}

void natiahniTabulku(){
    int route_sock,i,j;
    route_request *request = (route_request *)malloc(sizeof(route_request));
    int retValue = -1,nbytes = 0,reply_len = 0;
    char reply_ptr[1024];
    ssize_t counter = 1024;
    int count =0;
    struct rtmsg *rtp;
    struct rtattr *rtap;
    struct nlmsghdr *nlp;
    int rtl;
    //struct RouteInfo route[24];
    char* buf = reply_ptr;
    unsigned long bufsize ;

    route_sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

    bzero(request,sizeof(route_request));

    // Fill in the NETLINK header
    request->nlMsgHdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    request->nlMsgHdr.nlmsg_type = RTM_GETROUTE;
    request->nlMsgHdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

    // set the routing message header
    request->rtMsg.rtm_family = AF_INET;
    request->rtMsg.rtm_table = 254;

    // Send routing request
    if ((retValue = send(route_sock, request, sizeof(route_request), 0)) < 0)
    {
    perror("send");
    exit(1);
    }

    for(;;)
    {
    /*if( counter < sizeof( struct nlmsghdr))
    {
    printf("Routing table is bigger than 1024\n");
    exit(1);
    }*/

    nbytes = recv(route_sock, &reply_ptr[reply_len], counter, 0);

    if(nbytes < 0 )
    {
    printf("Error in recv\n");
    break;
    }

    if(nbytes == 0)
    printf("EOF in netlink\n");

    nlp = (struct nlmsghdr*)(&reply_ptr[reply_len]);

    if (nlp->nlmsg_type == NLMSG_DONE)
    {
    // All data has been received.
    // Truncate the reply to exclude this message,
    // i.e. do not increase reply_len.
    break;
    }

    if (nlp->nlmsg_type == NLMSG_ERROR)
    {
    printf("Error in msg\n");
    exit(1);
    }

    reply_len += nbytes;
    counter -= nbytes;
    }

    /*======================================================*/
    bufsize = reply_len;
    // string to hold content of the route
    // table (i.e. one entry)
    unsigned int flags;

    // outer loop: loops thru all the NETLINK
    // headers that also include the route entry
    // header
    nlp = (struct nlmsghdr *) buf;

    for(i= -1; NLMSG_OK(nlp, bufsize); nlp=NLMSG_NEXT(nlp, bufsize))
    {
    // get route entry header
    rtp = (struct rtmsg *) NLMSG_DATA(nlp);
    // we are only concerned about the
    // tableId route table
    if(rtp->rtm_table != 254)
    continue;
    i++;

                
    // init all the strings
    bzero(&route[i], sizeof(struct RouteInfo*));
    flags = rtp->rtm_flags;
    route[i].proto = rtp->rtm_protocol;

    // inner loop: loop thru all the attributes of
    // one route entry
    rtap = (struct rtattr *) RTM_RTA(rtp);
    rtl = RTM_PAYLOAD(nlp);
    for( ; RTA_OK(rtap, rtl); rtap = RTA_NEXT(rtap, rtl))
    {
    switch(rtap->rta_type)
    {
    // destination IPv4 address
    case RTA_DST:
    count = 32 - rtp->rtm_dst_len;

    route[i].dstAddr = *(unsigned long *) RTA_DATA(rtap);

    route[i].mask = 0xffffffff;
    for (; count!=0 ;count--)
    route[i].mask = route[i].mask << 1;

    //printf("dst:%s \tmask:0x%x \t",inet_ntoa(route[i].dstAddr), route[i].mask);
    break;
    case RTA_GATEWAY:
    route[i].gateWay = *(unsigned long *) RTA_DATA(rtap);
    //printf("gw:%s\t",inet_ntoa(route[i].gateWay));
    break;
    case RTA_PREFSRC:
    route[i].srcAddr = *(unsigned long *) RTA_DATA(rtap);
    //printf("src:%s\t", inet_ntoa(route[i].srcAddr));
    break;
    // unique ID associated with the network
    // interface
    case RTA_OIF:
    ifname(*((int *) RTA_DATA(rtap)),route[i].ifName);
    //printf( "ifname %s\n", route[i].ifName);
    break;
    default:
    break;
    }

    }
    //set Flags

    //[TODO]: UP hardcoded?!
    route[i].flags|=RTF_UP;
    if (route[i].gateWay != 0)
    route[i].flags|=RTF_GATEWAY;
    if (route[i].mask == 0xFFFFFFFF)
    route[i].flags|=RTF_HOST;
    }

    pocetRouteZaznamov = i;

    // Print the route records
    printf("Destination\t\tGateway \t\tNetmask \t\tflags \t\tIfname \n");
    printf("-----------\t\t------- \t\t--------\t\t------\t\t------ \n");
    for( j = 0; j<= i; j++)
    {
    struct in_addr pom;
    char pole[100];

    pom.s_addr = route[j].dstAddr;
    strcpy(pole,inet_ntoa(pom));
    printf("%-18s",pole);
    pom.s_addr = route[j].gateWay;
    strcpy(pole,inet_ntoa(pom));
    printf("\t%-18s",pole);
    route[j].mask = ntohl(route[j].mask);
    pom.s_addr = route[j].mask;
    strcpy(pole,inet_ntoa(pom));
    printf("\t%-18s",pole);
    printf("\t%-10d",route[j].flags);
    printf("\t%-10s\n",route[j].ifName);

}
}

bool addRTE( char* paIP, char* paGATEWAY, char* paGENMASK, char* paETH )            
{ 
   // create the control socket.
   //int fd = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
   int fd = socket( AF_INET, SOCK_DGRAM, 0 );
   if(fd == -1){
       perror("Socket: ");
        return false;
   }

   struct rtentry route;

      memset( &route, 0, sizeof( route ) );
      /*
    strcpy(route.rt_dst.sa_data, IP);
    route.rt_dst.sa_family = AF_INET;
    strcpy(route.rt_gateway.sa_data, GATEWAY);
    strcpy(route.rt_genmask.sa_data, GENMASK);
    route.rt_dev = RTMSG_NEWDEVICE;
    */
/*
    inet_aton(IP, &route.rt_dst);
    inet_aton(GATEWAY, &route.rt_gateway);
    inet_aton(GENMASK, &route.rt_genmask);
   */


   // set the gateway to 0.
   struct sockaddr_in *addr = (struct sockaddr_in *)&route.rt_gateway;
   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = inet_addr(paGATEWAY);
    addr->sin_port = 0;

    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(paIP);
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;

    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(paGENMASK);
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;

    memcpy((void*) &route.rt_gateway, addr, sizeof(*addr));
    route.rt_dev = paETH;
    
/*
   // set the host we are rejecting. 
   addr = (struct sockaddr_in*) &route.rt_dst;
   addr->sin_family = AF_INET;
   addr->sin_addr.s_addr = htonl(host);
*/
   // Set the mask. In this case we are using 255.255.255.255, to block a single
   // IP. But you could use a less restrictive mask to block a range of IPs. 
   // To block and entire C block you would use 255.255.255.0, or 0x00FFFFFFF
   addr = (struct sockaddr_in*) &route.rt_genmask;
   addr->sin_family = AF_INET;
   //addr->sin_addr.s_addr = 0xFFFFFFFF;


   // These flags mean: this route is created "up", or active
   // The blocked entity is a "host" as opposed to a "gateway"
   // The packets should be rejected. On BSD there is a flag RTF_BLACKHOLE
   // that causes packets to be dropped silently. We would use that if Linux
   // had it. RTF_REJECT will cause the network interface to signal that the 
   // packets are being actively rejected.
   route.rt_flags = RTF_UP;
   route.rt_metric = 0;
    int p;
   // this is where the magic happens..
   if ( p = ioctl( fd, SIOCADDRT, &route ) )
   {
       printf("Succes %d\n",p);
       perror("IOCTL: \n");
      close( fd );
      return false;
   }

   // remember to close the socket lest you leak handles.
   close( fd );
   return true; 
}

char* getIPfromInterface(char* name){
  int fd;
  struct ifreq ifr;

 fd = socket(AF_INET, SOCK_DGRAM, 0);

 /* I want to get an IPv4 IP address */
 ifr.ifr_addr.sa_family = AF_INET;

 /* I want IP address attached to "eth0" */
 strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

 ioctl(fd, SIOCGIFADDR, &ifr);

 close(fd);

 /* display result */
 name = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
 return name;
}

int
main (void)
{
  int Socket;
  struct sockaddr_in MyAddr;
  struct ip_mreqn McastGroup;
  struct RIPMessage *RM;
  pthread_t TID;

///////////////////////////////////////////////////////////////////////////////////////////////NACITANIE LINUX SMEROVACEJ TABULKY

  natiahniTabulku();

///////////////////////////////////////////////////////////////////////////////////////////////
  if (inet_aton (RIP_GROUP, &(McastGroup.imr_multiaddr)) == 0)
    {
      fprintf (stderr,
	       "Error: %s is not a valid IPv4 address.\n\n", RIP_GROUP);
      exit (EXIT_ERROR);
    }

  McastGroup.imr_address.s_addr = INADDR_ANY;
  McastGroup.imr_ifindex = 0;
  //McastGroup.imr_ifindex = if_nametoindex ("veth1");
  if ((Socket = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror ("socket");
      exit (EXIT_ERROR);
    }

  MyAddr.sin_family = AF_INET;
  MyAddr.sin_port = htons (PORT);
  MyAddr.sin_addr.s_addr = INADDR_ANY;
  if (bind (Socket, (struct sockaddr *) &MyAddr, sizeof (MyAddr)) == -1)
    {
      perror ("bind");
      close (Socket);
      exit (EXIT_ERROR);
    }

  if (setsockopt
      (Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
       &McastGroup, sizeof (McastGroup)) == -1)
    {

      perror ("setsockopt");
      close (Socket);
      exit (EXIT_ERROR);
    }

  if ((RM = (struct RIPMessage *) malloc (MSGLEN)) == NULL)
    {
      perror ("malloc");
      close (Socket);
      exit (EXIT_ERROR);
    }

  pthread_create (&TID, NULL, SenderThread, &Socket);

  for (;;)
    {
      struct sockaddr_in SenderAddr;
      socklen_t AddrLen;
      struct RIPNetEntry *E;
      ssize_t BytesRead;
      ssize_t BytesProcessed;
      BytesProcessed = 0;
      memset (RM, 0, MSGLEN);
      AddrLen = sizeof (SenderAddr);
      if ((BytesRead =
	   recvfrom (Socket, RM, MSGLEN, 0,
		     (struct sockaddr *) &SenderAddr, &AddrLen)) == -1)
	{
	  perror ("recvfrom");
	  close (Socket);
	  exit (EXIT_ERROR);
	}

      if (((BytesRead -
	    sizeof (struct RIPMessage)) % sizeof (struct RIPNetEntry)) != 0)
	{
	  printf
	    ("Corrupted (truncated) RIP message from %s\n",
	     inet_ntoa (SenderAddr.sin_addr));
	  continue;
	}

      if (BytesRead < sizeof (struct RIPMessage))
	{
	  printf
	    ("Corrupted RIP message with incomplete header from %s\n",
	     inet_ntoa (SenderAddr.sin_addr));
	  continue;
	}

      if (RM->Command != RIP_M_RESP)
	{
	  printf ("Unknown RIP message type %hhu from %s\n",
		  RM->Command, inet_ntoa (SenderAddr.sin_addr));
	  continue;
	}

      if (RM->Version != RIP_VERSION)
	{
	  printf
	    ("Unknown RIP message version %hhu from %s\n",
	     RM->Version, inet_ntoa (SenderAddr.sin_addr));
	  continue;
	}

      BytesProcessed += sizeof (struct RIPMessage);
      E = RM->Entry;

      char interface[10] = ETH;
      strcpy(interface,getIPfromInterface(interface));
      if( strcmp(interface, inet_ntoa (SenderAddr.sin_addr)) == 0){
          continue;
      }
      printf ("RIP message from %s:\n", inet_ntoa (SenderAddr.sin_addr));
      while (BytesProcessed < BytesRead)
	{
	  if (ntohs (E->AF) == AF_INET)
	    {
	      char Network[IPTXTLEN];
	      char Netmask[IPTXTLEN];
	      char NextHop[IPTXTLEN];
        char viaETH[IPTXTLEN] = ETH;
	      memset (Network, '\0', IPTXTLEN);
	      memset (Netmask, '\0', IPTXTLEN);
	      memset (NextHop, '\0', IPTXTLEN);
	      strncpy (Network, inet_ntoa (E->Net), IPTXTLEN - 1);
	      strncpy (Netmask, inet_ntoa (E->Mask), IPTXTLEN - 1);
	      strncpy (NextHop, inet_ntoa (E->NHop), IPTXTLEN - 1);
	      printf ("\t%s/ %s, metric=%u, nh=%s, tag=%hu\n",
		      Network, Netmask, ntohl (E->Metric),
		      NextHop, ntohs (E->Tag));
              addRTE(Network, NextHop, Netmask, viaETH);

	    }
	  else
	    printf ("\tUnknown address family %hu\n", ntohs (E->AF));
	  BytesProcessed += sizeof (struct RIPNetEntry);
	  E++;
	}
    }

  close (Socket);

  exit (EXIT_SUCCESS);
}
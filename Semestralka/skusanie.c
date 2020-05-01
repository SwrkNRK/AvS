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
#define     ETH "eth2"

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

struct ifreq ifaces[10];
int pocetIfaces = 0;
struct RouteInfo route[25];
int routeCount = 0;
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

bool delRTE( char* paIP, char* paGATEWAY, char* paGENMASK, char* paETH, bool useGateway )            
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
      

    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(paIP);
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;

    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(paGENMASK);
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;


  ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(paGATEWAY);
  ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
  
   route.rt_dev = paETH;
   route.rt_metric = 1;

  if(useGateway){ route.rt_flags = RTF_UP | RTF_GATEWAY;
  } else {route.rt_flags = RTF_UP;}

    int p;
   // this is where the magic happens..
   if ( p = ioctl( fd, SIOCDELRT, &route ) )
   {
       /*printf("Succes %d\n",p);
       perror("IOCTL: \n");
      close( fd );
      return false;*/
   }

   // remember to close the socket lest you leak handles.
   close( fd );
   return true; 
}

bool addRTE( char* paIP, char* paGATEWAY, char* paGENMASK, char* paETH, bool useGateway )            
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
      

    ((struct sockaddr_in *)&route.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_dst)->sin_addr.s_addr = inet_addr(paIP);
    ((struct sockaddr_in *)&route.rt_dst)->sin_port = 0;

    ((struct sockaddr_in *)&route.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_genmask)->sin_addr.s_addr = inet_addr(paGENMASK);
    ((struct sockaddr_in *)&route.rt_genmask)->sin_port = 0;


  ((struct sockaddr_in *)&route.rt_gateway)->sin_family = AF_INET;
    ((struct sockaddr_in *)&route.rt_gateway)->sin_addr.s_addr = inet_addr(paGATEWAY);
  ((struct sockaddr_in *)&route.rt_gateway)->sin_port = 0;
  
   route.rt_dev = paETH;
   route.rt_metric = 1;

  if(useGateway){ route.rt_flags = RTF_UP | RTF_GATEWAY;
  } else {route.rt_flags = RTF_UP;}

    int p;
   // this is where the magic happens..
   if ( p = ioctl( fd, SIOCADDRT, &route ) )
   {
       /*printf("Succes %d\n",p);
       perror("IOCTL: \n");
      close( fd );
      return false;*/
   }

   // remember to close the socket lest you leak handles.
   close( fd );
   return true; 
}


void *
SenderThread (void *Arg)
{
  int Socket = *((int *) Arg);
  struct RIPMessage *RM = (struct RIPMessage *) malloc (MSGLEN);
  int BytesToSend;
  struct sockaddr_in DstAddr;
  struct timespec TimeOut;
  struct in_addr sock_opt_addr;

  char MNetwork[IPTXTLEN] = "224.0.0.0";
	char MNetmask[IPTXTLEN] = "255.255.255.0";
	char MNextHop[IPTXTLEN] = "0.0.0.0";
  char MviaETH[IPTXTLEN];

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

      for (int i = 0; i < routeCount-1; i++)                                       //POSIELANIE
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
		  perror ("SenderThread sendto1");
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

  for(int i=0; i < pocetIfaces-1; i++){
  char *pom = inet_ntoa(((struct sockaddr_in *)&ifaces[i].ifr_addr)->sin_addr);
  sock_opt_addr.s_addr = inet_addr(pom);
  setsockopt(Socket, IPPROTO_IP, IP_MULTICAST_IF, &sock_opt_addr, sizeof(sock_opt_addr));


      if (sendto
	  (Socket, RM, BytesToSend, 0, (struct sockaddr *) &DstAddr,
	   sizeof (DstAddr)) == -1)
	{
	  perror ("SenderThread sendto2");
	  close (Socket);
	  exit (EXIT_ERROR);
	}
  }
    }
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

void createIFaceTable(){
  int fd;
  int i = 0;

  char name[10] = "eth1";
  char *pom = "asdas";
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  while(strcmp(pom, "0.0.0.0") != 0){

  /* I want to get an IPv4 IP address */
  ifaces[i].ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "eth0" */
  strncpy(ifaces[i].ifr_name, name, IFNAMSIZ-1);

  ioctl(fd, SIOCGIFADDR, &ifaces[i]);

  pom = inet_ntoa(((struct sockaddr_in *)&ifaces[i].ifr_addr)->sin_addr);

  pocetIfaces++;
  name[3]++;
  i++;

  }

  close(fd);
}


void loadConfig(FILE* config){
int c = fgetc(config);
int i=0;
char buf[20];
memset(buf, '\0', sizeof(buf));

while (c != EOF)
{
  buf[i] = c;
  i++;
  //IP
  if(c == ' '){
    route[routeCount].dstAddr = inet_addr(buf);
    memset(buf, '\0', sizeof(buf));
    i=0;
  }
  //MASK
  if(c == '\n' || c == EOF){
    route[routeCount].mask = inet_addr(buf);
    memset(buf, '\0', sizeof(buf));
    i=0;
    routeCount++;
  }

    c = fgetc(config);
}

if(c == '\n' || c == EOF){
    route[routeCount].mask = inet_addr(buf);
    memset(buf, '\0', sizeof(buf));
    i=0;
    routeCount++;
  }

}

int
main (int argc, char *argv[])
{
  if(argc < 2){
    printf("Je potrebne zadat parameter nazov txt suboru v ktorom je config\n");
    return EXIT_FAILURE;
  }
  printf("%s\n",argv[1]);
FILE* conf = fopen(argv[1], "r");
loadConfig(conf);
fclose(conf);

  int Socket;
  struct sockaddr_in MyAddr;
  struct ip_mreqn McastGroup;
  struct RIPMessage *RM;
  pthread_t TID;

  createIFaceTable();
  char *pom;
  for(int i=0; i < pocetIfaces-1; i++){
    pom = inet_ntoa(((struct sockaddr_in *)&ifaces[i].ifr_addr)->sin_addr);
    printf("%s : %s\n",pom,ifaces[i].ifr_ifrn.ifrn_name);
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

        if (inet_aton (RIP_GROUP, &(McastGroup.imr_multiaddr)) == 0)
    {
      fprintf (stderr,
	       "Error: %s is not a valid IPv4 address.\n\n", RIP_GROUP);
      exit (EXIT_ERROR);
    }
      
  if (setsockopt
      (Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
       &McastGroup, sizeof (McastGroup)) == -1)
    {

      perror ("setsockopt2");
      close (Socket);
      exit (EXIT_ERROR);
    }

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
	      strncpy (NextHop, inet_ntoa (SenderAddr.sin_addr), IPTXTLEN - 1);
	      printf ("\t%s/ %s, metric=%u, nh=%s, tag=%hu\n",
		      Network, Netmask, ntohl (E->Metric),
		      NextHop, ntohs (E->Tag));
              addRTE(Network, NextHop, Netmask, viaETH, true);

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
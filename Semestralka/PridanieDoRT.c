#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define ETH "eth2"
#define IP "192.168.5.0"
#define GATEWAY "0.0.0.0"
#define GENMASK "255.255.255.0"

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

int main (){
    char Network[20] = IP;
	      char gateway[20] = GATEWAY;
	      char genmask[20] = GENMASK;
          char viaETH[20] = ETH;
    addNullRoute(Network, gateway, genmask, viaETH);
    printf("Exit Success\n");
    return EXIT_SUCCESS;
}
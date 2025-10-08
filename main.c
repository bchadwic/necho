#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define N 200

const char ECHO_MESSAGE = 8;
const char DEFAULT_CODE = 0;

void message(uint8_t *buf, size_t n);
uint16_t checksum(uint8_t *buf, size_t n);
void fill_str(uint8_t *buf, uint8_t i, char *v);
void fill_uint16(uint8_t *buf, uint8_t i, uint16_t v);

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
int main() {
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    uint8_t buf[N] = {0};
    message(buf, N);

    struct sockaddr_in dst;

    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr("127.0.0.1");

    ssize_t n;
    n = sendto(fd, buf, N, 0, (struct sockaddr *)&dst, sizeof(dst));
    if (n == -1) {
        perror("sendto");
        return 1;
    }

    uint8_t resp[N] = {0};

    struct sockaddr_in src;
    memset(&src, 0, sizeof(src));
    socklen_t srclen = sizeof(src);

    n = recvfrom(fd, resp, N, 0, (struct sockaddr*)&src, &srclen);
    if (n == -1) {
        perror("recvfrom");
        return 1;
    }

    printf("Received %zd bytes from %s:%d\n",
        n, inet_ntoa(src.sin_addr), ntohs(src.sin_port));


    for (int i = 0; i < n; i++) {
        unsigned char c = resp[i];

        // Print hex value
        printf("%02X ", c);

        // After every 16 bytes, print ASCII equivalents on the side
        if ((i + 1) % 16 == 0 || i == n - 1) {
            int start = i - (i % 16);
            printf(" | ");
            for (int j = start; j <= i; j++) {
                unsigned char ch = resp[j];
                printf("%c", (ch >= 32 && ch <= 126) ? ch : '.');
            }
            printf("\n");
        }
    }

    close(fd);
    return 0;
}

void message(uint8_t* buf, size_t n) {
    buf[0] = ECHO_MESSAGE;
    buf[1] = DEFAULT_CODE;
   
    // get pid
    uint16_t identifier = 10;
    fill_uint16(buf, 4, identifier);
   
    // increment as we go
    uint16_t seq_num = 1;
    fill_uint16(buf, 6, seq_num);

    char *data = "hello world!";
    fill_str(buf, 8, data);

    uint16_t sum = checksum(buf, n);
    fill_uint16(buf, 2, sum);
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

void fill_str(uint8_t *buf, uint8_t i, char *v) {
    for (int j = 0; v[j] != '\0'; i++, j++) {
        buf[i] = v[j];
    }
}

void fill_uint16(uint8_t *buf, uint8_t i, uint16_t v) {
    buf[i] = (v >> 8) & 0xFF;
    buf[i + 1] = v & 0xFF;
}

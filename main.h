#include <string.h>
#include <stdint.h>

char* message(int argc, char **argv);
int necho(char *dst_host, char *msg);
void packet(uint8_t *buf, size_t n, char *msg);
uint16_t checksum(uint8_t *buf, size_t n);
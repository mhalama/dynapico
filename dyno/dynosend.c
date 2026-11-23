/* dynosend IP file.bin : sends [header|code] and prints the device's reply */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dyno_build.h"

#define PORT 3333
#define MAGIC 0x4f4e5944u

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "Usage: %s IP file.bin\n", argv[0]); return 2; }

    FILE *f = fopen(argv[2], "rb");
    if (!f) { perror(argv[2]); return 1; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fprintf(stderr, "empty file\n"); return 1; }

    uint8_t *buf = malloc(16 + (size_t)n);
    uint32_t hdr[4] = { MAGIC, (uint32_t)n, DYNO_BUILD_BASE, DYNO_BUILD_SIG };   /* little-endian host */
    memcpy(buf, hdr, 16);
    if (fread(buf + 16, 1, (size_t)n, f) != (size_t)n) { perror("fread"); return 1; }
    fclose(f);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(PORT) };
    if (inet_pton(AF_INET, argv[1], &a.sin_addr) != 1) { fprintf(stderr, "bad IP address\n"); return 2; }
    if (connect(s, (struct sockaddr *)&a, sizeof a) != 0) { perror("connect"); return 1; }

    for (size_t off = 0; off < 16 + (size_t)n;) {
        ssize_t w = write(s, buf + off, 16 + (size_t)n - off);
        if (w <= 0) { perror("write"); return 1; }
        off += (size_t)w;
    }
    shutdown(s, SHUT_WR);                       /* FIN = end of block */

    char r[512];
    ssize_t k;
    while ((k = read(s, r, sizeof r)) > 0) fwrite(r, 1, (size_t)k, stdout);
    close(s);
    return 0;
}

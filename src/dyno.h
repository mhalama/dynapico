#pragma once
#include <stddef.h>
#include <stdint.h>
#include "dyno_api.h"

#define DYNO_MAGIC        0x4f4e5944u   /* "DYNO" little endian */
#define DYNO_EXEC_BUF_LEN 16384
#define DYNO_MEM_WORDS    100
#define DYNO_REPLY_LEN    1024

/* Hlavicka pred kodem, little endian, 16 B */
typedef struct {
    uint32_t magic;
    uint32_t len;     /* delka kodu za hlavickou */
    uint32_t base;    /* adresa exec_buf, pro kterou byl kod prelozen */
    uint32_t sig;     /* exec_buf ^ dyno_runner - shoda s buildem firmwaru */
} dyno_header_t;

/* Overi a spusti blok [hlavicka|kod]. Do reply zapise text odpovedi (vzdy zakonceny \0). */
int dyno_run(const uint8_t *data, size_t len, char *reply, size_t reply_size);
uint32_t dyno_sig(void);

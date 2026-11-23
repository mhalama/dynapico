#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "dyno_api.h"

#define DYNO_MAGIC        0x4f4e5944u   /* "DYNO" little endian */
#define DYNO_EXEC_BUF_LEN 16384
#define DYNO_MEM_WORDS    100
#define DYNO_REPLY_LEN    256
#define DYNO_CORE1_STACK_WORDS 2048   /* 8 kB stack of the program on core 1 */

/* Header preceding the code, little endian, 16 B */
typedef struct {
    uint32_t magic;
    uint32_t len;     /* length of the code following the header */
    uint32_t base;    /* address of exec_buf the code was built for */
    uint32_t sig;     /* exec_buf ^ dyno_run - matches the firmware build */
} dyno_header_t;

/* Validates a [header|code] block, stops the previous program and starts the new one on core 1.
   Does NOT wait for its result; reply only receives an acknowledgement/error (always NUL-terminated). */
int dyno_run(const uint8_t *data, size_t len, char *reply, size_t reply_size);
uint32_t dyno_sig(void);
void dyno_poll(void);            /* call from the main loop */
bool dyno_core1_running(void);

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TCP_SERVER_PORT    3333
#define TCP_SERVER_MAX_LEN (16 * 1024 + 64)
#define TCP_SERVER_IDLE_S  10

bool tcp_server_start(void);

/* Returns true when the received block is complete (the client finished sending). */
bool tcp_server_poll(const uint8_t **data, size_t *len);

/* Sends the reply to the client, closes the connection and frees the server for the next block. */
void tcp_server_finish(const char *reply, size_t reply_len);

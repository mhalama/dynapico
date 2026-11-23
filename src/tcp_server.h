#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TCP_SERVER_PORT    3333
#define TCP_SERVER_MAX_LEN (16 * 1024 + 64)
#define TCP_SERVER_IDLE_S  10

bool tcp_server_start(void);

/* Vrati true, pokud je prijaty blok kompletni (klient ukoncil odesilani). */
bool tcp_server_poll(const uint8_t **data, size_t *len);

/* Posle odpoved klientovi, zavre spojeni a uvolni server pro dalsi blok. */
void tcp_server_finish(const char *reply, size_t reply_len);

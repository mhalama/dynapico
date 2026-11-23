/*
    Simple TCP server on top of the lwIP raw API. The callbacks run in the lwIP
    context (cyw43 background IRQ/worker), so they only collect bytes. The code
    is started from the main loop (tcp_server_poll/finish).
*/
#include <string.h>
#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "tcp_server.h"

enum { ST_IDLE, ST_RECEIVING, ST_READY, ST_RUNNING };

static struct tcp_pcb *s_listen;
static struct tcp_pcb *s_client;
static volatile int s_state = ST_IDLE;
static uint8_t s_buf[TCP_SERVER_MAX_LEN];
static volatile size_t s_len;

static void client_drop(bool abort_conn)
{
    if (!s_client) return;
    struct tcp_pcb *c = s_client;
    s_client = NULL;
    tcp_arg(c, NULL); tcp_recv(c, NULL); tcp_err(c, NULL); tcp_poll(c, NULL, 0); tcp_sent(c, NULL);
    if (abort_conn || tcp_close(c) != ERR_OK) tcp_abort(c);
    s_state = ST_IDLE;
}

static err_t on_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    (void)arg;
    if (!p) {                       /* FIN from the client = end of block */
        if (s_state == ST_RECEIVING) s_state = ST_READY;
        return ERR_OK;
    }
    if (err != ERR_OK || s_state != ST_RECEIVING) { pbuf_free(p); return ERR_OK; }

    if (s_len + p->tot_len > sizeof s_buf) {
        printf("tcp: block too long\n");
        pbuf_free(p);
        client_drop(true);
        return ERR_ABRT;
    }
    s_len += pbuf_copy_partial(p, s_buf + s_len, p->tot_len, 0);
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void on_err(void *arg, err_t err)
{
    (void)arg; (void)err;
    s_client = NULL;                /* the pcb has already been freed by lwIP */
    if (s_state != ST_RUNNING) s_state = ST_IDLE;
}

static err_t on_poll(void *arg, struct tcp_pcb *pcb)
{
    (void)arg; (void)pcb;           /* called every 2 s; client is silent while receiving */
    static int idle;
    if (s_state == ST_RECEIVING && ++idle >= TCP_SERVER_IDLE_S / 2) {
        idle = 0;
        printf("tcp: timeout\n");
        client_drop(true);
        return ERR_ABRT;
    }
    if (s_state != ST_RECEIVING) idle = 0;
    return ERR_OK;
}

static err_t on_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    (void)arg;
    if (err != ERR_OK || !newpcb) return ERR_VAL;
    if (s_state != ST_IDLE) {       /* busy: reject the new client */
        tcp_abort(newpcb);
        return ERR_ABRT;
    }
    s_client = newpcb;
    s_len = 0;
    s_state = ST_RECEIVING;
    tcp_arg(newpcb, NULL);
    tcp_recv(newpcb, on_recv);
    tcp_err(newpcb, on_err);
    tcp_poll(newpcb, on_poll, 4);
    return ERR_OK;
}

bool tcp_server_start(void)
{
    cyw43_arch_lwip_begin();
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    bool ok = pcb && tcp_bind(pcb, IP_ANY_TYPE, TCP_SERVER_PORT) == ERR_OK;
    if (ok) {
        s_listen = tcp_listen_with_backlog(pcb, 1);
        ok = s_listen != NULL;
        if (ok) tcp_accept(s_listen, on_accept);
    } else if (pcb) {
        tcp_close(pcb);
    }
    cyw43_arch_lwip_end();
    return ok;
}

bool tcp_server_poll(const uint8_t **data, size_t *len)
{
    if (s_state != ST_READY) return false;
    s_state = ST_RUNNING;
    *data = s_buf;
    *len = s_len;
    return true;
}

void tcp_server_finish(const char *reply, size_t reply_len)
{
    cyw43_arch_lwip_begin();
    if (s_client) {
        if (reply_len && reply_len <= tcp_sndbuf(s_client)) {
            tcp_write(s_client, reply, (u16_t)reply_len, TCP_WRITE_FLAG_COPY);
            tcp_output(s_client);
        }
        client_drop(false);         /* tcp_close sends the remaining data and FIN */
    }
    s_state = ST_IDLE;
    cyw43_arch_lwip_end();
}

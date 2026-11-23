#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "tcp_server.h"
#include "dyno.h"

#ifndef WIFI_SSID
#error "Zadej -DWIFI_SSID=... -DWIFI_PASSWORD=... pri cmake"
#endif

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);                 /* cas na pripojeni USB konzole */

    if (cyw43_arch_init()) {
        printf("cyw43_arch_init selhalo\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Pripojuji k WiFi \"%s\"...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Pripojeni k WiFi selhalo\n");
        return 1;
    }
    printf("IP adresa: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    if (!tcp_server_start()) {
        printf("tcp_server_start selhal\n");
        return 1;
    }
    printf("Server posloucha na portu %d\n", TCP_SERVER_PORT);

    static char reply[DYNO_REPLY_LEN];
    for (;;) {
        const uint8_t *data;
        size_t len;
        if (tcp_server_poll(&data, &len)) {
            printf("Prijato %u B\n", (unsigned)len);
            dyno_run(data, len, reply, sizeof reply);
            tcp_server_finish(reply, strlen(reply));
        }
        dyno_poll();
        sleep_ms(1);                /* WiFi/lwIP bezi na pozadi (threadsafe_background) */
    }
}

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "tcp_server.h"
#include "dyno.h"

#ifndef WIFI_SSID
#error "Pass -DWIFI_SSID=... -DWIFI_PASSWORD=... to cmake"
#endif

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);                 /* time to attach the USB console */

    if (cyw43_arch_init()) {
        printf("cyw43_arch_init failed\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi \"%s\"...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connection failed\n");
        return 1;
    }
    printf("IP address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    if (!tcp_server_start()) {
        printf("tcp_server_start failed\n");
        return 1;
    }
    printf("Server listening on port %d\n", TCP_SERVER_PORT);

    static char reply[DYNO_REPLY_LEN];
    for (;;) {
        const uint8_t *data;
        size_t len;
        if (tcp_server_poll(&data, &len)) {
            printf("Received %u B\n", (unsigned)len);
            dyno_run(data, len, reply, sizeof reply);
            tcp_server_finish(reply, strlen(reply));
        }
        dyno_poll();
        sleep_ms(1);                /* WiFi/lwIP run in the background (threadsafe_background) */
    }
}

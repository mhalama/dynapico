#include "dyno.h"

/* Nekonecna smycka na jadre 1; dalsi nahrani ji zastavi */
MAIN_ENTRY(pamet)
{
    int n = 0;
    for (;;) {
        led(1); delay(100);
        led(0); delay(400);
        pamet[0] = (uint32_t)++n;
        if (n % 10 == 0)
            loguj_n("blikl jsem ", n);
    }
    return 0;
}

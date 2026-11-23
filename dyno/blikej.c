#include "dyno.h"

/* Infinite loop on core 1; the next upload stops it */
MAIN_ENTRY(mem)
{
    int n = 0;
    for (;;) {
        led(1); delay(100);
        led(0); delay(400);
        mem[0] = (uint32_t)++n;
        if (n % 10 == 0)
            loguj_n("blinked ", n);
    }
    return 0;
}

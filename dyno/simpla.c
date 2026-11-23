#include "dyno.h"

#define BLINK_DELAY 50

MAIN_ENTRY(pamet)
{
    loguj_n("Pocet spusteni: ", (int)++pamet[0]);
    for (int i = 0; i < 4; i++) {
        led(1); delay(BLINK_DELAY);
        led(0); delay(BLINK_DELAY);
    }
    return (int)pamet[0];
}

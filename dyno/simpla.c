#include "dyno.h"

#define COUNT 10
#define BLINK_DELAY 250

MAIN_ENTRY(mem)
{
    loguj_n("Run count: ", (int)++mem[0]);
    for (int i = 0; i < COUNT; i++) {
        led(1); delay(BLINK_DELAY);
        led(0); delay(BLINK_DELAY);
    }
    return (int)mem[0];
}

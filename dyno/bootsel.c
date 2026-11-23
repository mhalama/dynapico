#include "dyno.h"

/* Vzdy do USB bootloaderu, tlacitko neni potreba. */
MAIN_ENTRY(pamet)
{
    (void)pamet;
    loguj("bootsel.c: do bootloaderu");
    delay(200);
    bootloader();
    return 0;
}

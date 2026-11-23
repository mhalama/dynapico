#include "dyno.h"

/* Zamerny HardFault na jadre 1 - jadro 0 a socket server musi dal fungovat */
MAIN_ENTRY(pamet)
{
    loguj("pad.c: zapis na adresu 0");
    delay(100);
    *(volatile uint32_t *)0xF0000000 = pamet[0];
    return 0;
}

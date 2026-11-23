#include "dyno.h"

/* Deliberate HardFault on core 1 - core 0 and the socket server must keep working */
MAIN_ENTRY(mem)
{
    loguj("pad.c: write to address 0xF0000000");
    delay(100);
    *(volatile uint32_t *)0xF0000000 = mem[0];
    return 0;
}

#include "dyno.h"

/* Always enters the USB bootloader, no button needed. */
MAIN_ENTRY(mem)
{
    (void)mem;
    loguj("bootsel.c: entering bootloader");
    delay(200);
    bootloader();
    return 0;
}

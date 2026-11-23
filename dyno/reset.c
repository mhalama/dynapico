#include "dyno.h"

/*
    Restarts the device. If BOOTSEL is held during the restart, the bootrom
    stays in the USB bootloader (RP2350 drive) - the program does not have to read the button.
    Procedure: hold BOOTSEL, run ./dyno.sh reset, release once the drive appears.
*/
MAIN_ENTRY(mem)
{
    (void)mem;
    loguj("reset.c: restart");
    delay(200);                         /* lets core 0 send the reply to the client */
    restart();
    return 0;
}

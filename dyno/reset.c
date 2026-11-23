#include "dyno.h"

/*
    Restart zarizeni. Pokud je pri restartu drzen BOOTSEL, bootrom zustane
    v USB bootloaderu (disk RP2350) - tlacitko nemusi program cist.
    Postup: drz BOOTSEL, ./dyno.sh reset, pust az se objevi disk.
*/
MAIN_ENTRY(pamet)
{
    (void)pamet;
    loguj("reset.c: restart");
    delay(200);                         /* jadro 0 stihne poslat odpoved klientovi */
    restart();
    return 0;
}

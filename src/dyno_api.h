#pragma once
/*
    Firmware API available to dynamically uploaded code (the programs in dyno/).

    Declare each function with a prototype and mark it DYNO_EXPORT(name).
      - In the firmware, DYNO_EXPORT keeps the compiler from dropping the function (used);
        CMake protects it from the linker (--undefined).
      - In the uploaded programs it expands to nothing; the prep tool generates
        #define name ((typeof(name) *)0xaddress) from the symbol table of build/dynapico.elf.
*/

#include <stdint.h>

#ifdef DYNO_FIRMWARE
#define DYNO_EXPORT(fn) extern typeof(fn) fn __attribute__((used))
#else
#define DYNO_EXPORT(fn) _Static_assert(1, "DYNO_EXPORT")
#endif

void debug(void);
DYNO_EXPORT(debug);

int loguj(const char *msg);
DYNO_EXPORT(loguj);

int loguj_n(const char *msg, int n);
DYNO_EXPORT(loguj_n);

void delay(int ms);
DYNO_EXPORT(delay);

/* GPIO (pin number = GP number; the on-board LED of the Pico 2 W is on the CYW43, not on a GPIO) */
void gpio_vystup(int pin, int hodnota);
DYNO_EXPORT(gpio_vystup);

int gpio_vstup(int pin);
DYNO_EXPORT(gpio_vstup);

void led(int zap);
DYNO_EXPORT(led);

uint32_t cas_ms(void);
DYNO_EXPORT(cas_ms);

/* enables (zap=1) / disables the pull-up on an input pin; call before gpio_vstup */
void gpio_pullup(int pin, int zap);
DYNO_EXPORT(gpio_pullup);

/* restart the chip (firmware starts again, WiFi reconnects) */
void restart(void);
DYNO_EXPORT(restart);

/* reboot into the USB bootloader (BOOTSEL, RP2350 drive) */
void bootloader(void);
DYNO_EXPORT(bootloader);

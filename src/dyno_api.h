#pragma once
/*
    API firmwaru dostupne dynamicky nahravanemu kodu (dyno/ (programy .c)).

    Kazda funkce se deklaruje prototypem a oznaci DYNO_EXPORT(nazev).
      - ve firmwaru DYNO_EXPORT zajisti, ze funkci neodstrani kompilator ani linker (used),
      - v dyno/ (programy .c) je prazdne; nastroj prep podle nazvu vygeneruje
        #define nazev ((typeof(nazev) *)0xadresa) z tabulky symbolu build/dynapico.elf.
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

/* GPIO (cislo pinu = GP cislo; palubni LED Pico 2 W je na CYW43, ne na GPIO) */
void gpio_vystup(int pin, int hodnota);
DYNO_EXPORT(gpio_vystup);

int gpio_vstup(int pin);
DYNO_EXPORT(gpio_vstup);

void led(int zap);
DYNO_EXPORT(led);

uint32_t cas_ms(void);
DYNO_EXPORT(cas_ms);

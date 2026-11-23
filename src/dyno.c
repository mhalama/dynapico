#define DYNO_FIRMWARE
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "dyno.h"

/* Odpoved pro klienta: loguj* pisou do USB stdio i sem */
static char *s_reply;
static size_t s_reply_size, s_reply_len;

static void reply_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char line[128];
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    fputs(line, stdout);
    fputc('\n', stdout);
    if (s_reply && s_reply_len + (size_t)n + 2 <= s_reply_size) {
        memcpy(s_reply + s_reply_len, line, (size_t)n);
        s_reply_len += (size_t)n;
        s_reply[s_reply_len++] = '\n';
        s_reply[s_reply_len] = 0;
    }
}

/* ---- API exportovane do nahravaneho kodu ---- */

void debug(void)                     { reply_printf("DEBUG"); }
int  loguj(const char *msg)          { reply_printf("%s", msg); return 0; }
int  loguj_n(const char *msg, int n) { reply_printf("%s%d", msg, n); return 0; }
void delay(int ms)                   { sleep_ms((uint32_t)ms); }
uint32_t cas_ms(void)                { return to_ms_since_boot(get_absolute_time()); }

void gpio_vystup(int pin, int hodnota)
{
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, GPIO_OUT);
    gpio_put((uint)pin, hodnota != 0);
}

int gpio_vstup(int pin)
{
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, GPIO_IN);
    return gpio_get((uint)pin);
}

void led(int zap)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, zap != 0);
}

/* ---- spoustec ---- */

/* Spustitelna SRAM (RP2350 nema MPU nastavene, XN neni). Neni static: symbol hleda Makefile pres nm. */
__attribute__((aligned(16), used)) uint8_t exec_buf[DYNO_EXEC_BUF_LEN];

/* Trvaly stav mezi nahranimi (do restartu) */
static uint32_t memory[DYNO_MEM_WORDS];

uint32_t dyno_sig(void)
{
    return (uint32_t)(uintptr_t)exec_buf ^ (uint32_t)(uintptr_t)&dyno_run;
}

int dyno_run(const uint8_t *data, size_t len, char *reply, size_t reply_size)
{
    s_reply = reply;
    s_reply_size = reply_size;
    s_reply_len = 0;
    if (reply_size) reply[0] = 0;

    dyno_header_t h;
    if (len < sizeof h) { reply_printf("CHYBA: kratky blok (%u B)", (unsigned)len); return -1; }
    memcpy(&h, data, sizeof h);

    if (h.magic != DYNO_MAGIC)              { reply_printf("CHYBA: spatny magic"); return -1; }
    if (h.len != len - sizeof h)            { reply_printf("CHYBA: delka %u != %u", (unsigned)h.len, (unsigned)(len - sizeof h)); return -1; }
    if (h.len == 0 || h.len > DYNO_EXEC_BUF_LEN) { reply_printf("CHYBA: kod se nevejde (%u B, max %u)", (unsigned)h.len, DYNO_EXEC_BUF_LEN); return -1; }
    if (h.base != (uint32_t)(uintptr_t)exec_buf || h.sig != dyno_sig()) {
        reply_printf("CHYBA: kod je prelozen pro jiny build firmwaru - spust make");
        return -1;
    }

    memcpy(exec_buf, data + sizeof h, h.len);
    __asm volatile("dsb\n isb" ::: "memory");

    /* Thumb: bit 0 musi byt 1 */
    int (*f)(uint32_t *) = (int (*)(uint32_t *))((uintptr_t)exec_buf | 1u);
    int ret = f(memory);

    reply_printf("navratova hodnota: %d", ret);
    return ret;
}

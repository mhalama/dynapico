#define DYNO_FIRMWARE
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "dyno.h"

/* Odpoved pro klienta (jen chyby/potvrzeni spousteni); vystup programu loguj* jde jen na USB konzoli */
static char *s_reply;
static size_t s_reply_size, s_reply_len;

/* Vypis jen na USB konzoli - smi se volat z obou jader (kod bezi na jadre 1) */
static void log_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char line[128];
    int n = vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    fputs(line, stdout);
    fputc('\n', stdout);
}

/* Vypis na konzoli i do odpovedi klientovi - jen z jadra 0 (dyno_run) */
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

void debug(void)                     { log_printf("DEBUG"); }
int  loguj(const char *msg)          { log_printf("%s", msg); return 0; }
int  loguj_n(const char *msg, int n) { log_printf("%s%d", msg, n); return 0; }
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

void gpio_pullup(int pin, int zap)
{
    gpio_init((uint)pin);
    gpio_set_dir((uint)pin, GPIO_IN);
    if (zap) gpio_pull_up((uint)pin); else gpio_disable_pulls((uint)pin);
}

void restart(void)    { watchdog_reboot(0, 0, 10); for (;;) __wfe(); }
void bootloader(void) { reset_usb_boot(0, 0);      for (;;) __wfe(); }

void led(int zap)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, zap != 0);
}

/* ---- spoustec ---- */

/* Spustitelna SRAM (RP2350 nema MPU nastavene, XN neni). Neni static: symbol hleda Makefile pres nm. */
__attribute__((aligned(16), used)) uint8_t exec_buf[DYNO_EXEC_BUF_LEN];

/* Trvaly stav mezi nahranimi (do restartu) */
static uint32_t memory[DYNO_MEM_WORDS];

/* Program bezi na jadre 1; jadro 0 mezitim obsluhuje WiFi/socket */
enum { CORE1_IDLE, CORE1_RUNNING, CORE1_FINISHED };
static volatile int s_state = CORE1_IDLE;
static volatile int s_ret;
static uint32_t s_stack[DYNO_CORE1_STACK_WORDS] __attribute__((aligned(8)));

static void core1_entry(void)
{
    /* Thumb: bit 0 musi byt 1 */
    int (*f)(uint32_t *) = (int (*)(uint32_t *))((uintptr_t)exec_buf | 1u);
    s_ret = f(memory);
    __dmb();
    s_state = CORE1_FINISHED;
    for (;;) __wfe();
}

uint32_t dyno_sig(void)
{
    return (uint32_t)(uintptr_t)exec_buf ^ (uint32_t)(uintptr_t)&dyno_run;
}

bool dyno_core1_running(void)
{
    return s_state == CORE1_RUNNING;
}

/* Vola main loop: oznami, ze program na jadre 1 skoncil (vratil se z main) */
void dyno_poll(void)
{
    if (s_state == CORE1_FINISHED) {
        s_state = CORE1_IDLE;
        printf("Program na jadre 1 skoncil, navratova hodnota: %d\n", s_ret);
    }
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

    /* Predchozi program (i zaseknuty/spadly) se zastavi resetem jadra 1 */
    multicore_reset_core1();
    s_state = CORE1_IDLE;

    memcpy(exec_buf, data + sizeof h, h.len);
    __asm volatile("dsb\n isb" ::: "memory");

    s_state = CORE1_RUNNING;
    multicore_launch_core1_with_stack(core1_entry, s_stack, sizeof s_stack);

    reply_printf("spusteno na jadre 1 (%u B)", (unsigned)h.len);
    return 0;
}

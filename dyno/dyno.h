#pragma once
#include <stdint.h>
#include "../src/dyno_api.h"   /* prototypy funkci firmwaru */
#include "dyno_ptr.h"          /* vygenerovano (prep): adresy techto funkci */

#define MAIN_ENTRY(xmem) __attribute__((section(".start"))) int main(uint32_t *xmem)

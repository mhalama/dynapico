# dynapico – upload code over TCP to a Pico 2 W (RP2350)

Develop on the Raspberry Pi Pico 2 W without reflashing the firmware on every
change. The firmware runs permanently, receives a raw binary over TCP port
3333, copies it into RAM and runs it on **core 1**. The uploaded code is linked
at the fixed address of the RAM buffer and calls firmware functions through
absolute addresses. An edit → run cycle takes a fraction of a second.

Plain C on top of the pico-sdk; tested with SDK 2.3.1 on a Pico 2 W. This is a
proof of concept (see [Limitations and risks](#limitations-and-risks)).

A port of an earlier project of the same idea for the ESP32-C6.

## How it works

```
 PC                                                  Pico 2 W
 simpla.c ──arm-none-eabi-gcc + ld──► simpla.bin
 build/dynapico.elf ──nm──► dyno_ptr.h               tcp_server (lwIP raw API, :3333)
 dynosend IP simpla.bin ─[header|code]─────────────►   collects bytes into rx_buf
                                                       FIN from client = block complete
                                                     main loop: dyno_run()
                                                       validate header, stop core 1,
                                                       memcpy to exec_buf,
                                                       start exec_buf|1 on core 1
 acknowledgement ◄─────────────────────────────────  "started on core 1", socket closed
```

- **Firmware (core 0)** connects to WiFi and runs the TCP server. The lwIP
  callbacks only collect bytes; the code is started from the main loop.
- **Uploaded program (core 1)** runs for as long as it wants (it can be an
  infinite loop). The server does not wait for its result. The next upload
  stops it by resetting core 1 and starts the new program, so a program that
  hangs or crashes (HardFault) does not take down WiFi or the server.
- **Firmware API** is declared in `src/dyno_api.h` and marked
  `DYNO_EXPORT(name)`. The `prep` tool reads the addresses from
  `build/dynapico.elf` and generates `dyno/dyno_ptr.h`:

  ```c
  #define loguj ((typeof(loguj) *)0x100007b9)
  ```

  so the uploaded code simply calls `loguj("...")`; type checking comes from the
  prototype, the address from the macro.

### API available to uploaded code

| Function | Description |
|---|---|
| `void debug(void)` | Print "DEBUG" |
| `int loguj(const char *msg)` | Log a string (USB console) |
| `int loguj_n(const char *msg, int n)` | Log a string and a number |
| `void delay(int ms)` | `sleep_ms` |
| `uint32_t cas_ms(void)` | Milliseconds since boot |
| `void led(int on)` | On-board LED (on the CYW43) |
| `void gpio_vystup(int pin, int value)` | Drive a GPIO output |
| `int gpio_vstup(int pin)` | Read a GPIO input |
| `void gpio_pullup(int pin, int on)` | Pull-up on an input pin |
| `void restart(void)` | Restart the chip (firmware starts again) |
| `void bootloader(void)` | Reboot into the USB bootloader (BOOTSEL) |

To add a function: implement it in `src/dyno.c` and add a prototype with
`DYNO_EXPORT(name)` to `src/dyno_api.h`. CMake reads that list and keeps the
functions from being removed by `--gc-sections`.

### Building the uploaded code

The code is **linked directly at the address of `exec_buf`**: `dyno/Makefile`
finds the address and size in `build/dynapico.elf` (`nm -S`) and passes them
to the linker. The code therefore does not need to be position independent;
string literals, constant tables, globals (`.data`, `.bss`), function pointers
and `switch` jump tables all work. `.bss` is part of the binary as zeros (the
firmware clears nothing), so globals are initialised afresh with every upload.

Violations end as a link error instead of a silent runtime problem:

| Situation | Check | Error |
|---|---|---|
| Image larger than `exec_buf` (16 kB) | `MEMORY` region | `section '.image' will not fit in region 'EXEC'` |
| Entry function not at offset 0 | `ASSERT` | `use MAIN_ENTRY()` |
| Compiler emits a section the script does not handle | `--orphan-handling=error` | `unplaced orphan section` |
| Call to a function not in the image (`memset`, `printf`…) | undefined symbol | `undefined reference to ...` |
| Any linker warning | `--fatal-warnings` | – |

Helper functions of the compiler (64-bit division etc.) come from `libgcc`.

### Protocol

The client sends `[header 16 B | code]` and shuts down its sending side
(FIN). The header (little endian) is `magic "DYNO"`, `len`, `base` (address of
`exec_buf`) and `sig` (`exec_buf ^ dyno_run`). The firmware checks magic, length,
`base` and `sig` (code built against this firmware build), then starts the
program and replies with `started on core 1 (N B)` or an `ERROR: ...` line.

## Repository layout

| Path | Purpose |
|---|---|
| `CMakeLists.txt`, `build.sh` | Firmware build; WiFi credentials come from `.env` |
| `src/main.c` | WiFi, server start, main loop |
| `src/tcp_server.c/.h` | lwIP raw-API TCP server |
| `src/dyno.c/.h` | Header check, copy to `exec_buf`, start on core 1, API implementation |
| `src/dyno_api.h` | Firmware API visible to uploaded code (`DYNO_EXPORT`) |
| `src/lwipopts.h` | lwIP configuration (NO_SYS) |
| `dyno/prep.c` | PC tool: generates `dyno_ptr.h` and `dyno_build.h` from `build/dynapico.elf` |
| `dyno/dynosend.c` | PC client: adds the header, sends, prints the reply |
| `dyno/linker.ld`, `Makefile`, `dyno.sh` | Build `.c` → `.bin` at the address of `exec_buf` |
| `dyno/*.c` | Sample programs (`simpla`, `blikej`, `pad`, `reset`, `bootsel`) |

## Requirements

- `arm-none-eabi-gcc` ≥ 13 (Cortex-M33 support), newlib, cmake, ninja
- pico-sdk **2.x** with the `cyw43-driver`, `lwip` and `tinyusb` submodules
  (`PICO_SDK_PATH`, `build.sh` defaults to `~/pico/pico-sdk`)
- `libusb-1.0-0-dev`, `pkg-config` (the SDK builds picotool itself on first use)

```sh
git clone --branch 2.3.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init --depth 1 lib/cyw43-driver lib/lwip lib/tinyusb
```

## Usage

```sh
# .env (not in git):
#   WIFI_SSID=...
#   WIFI_PASSWORD=...
./build.sh                       # firmware -> build/dynapico.uf2
```

Hold BOOTSEL while plugging in the USB cable and copy `build/dynapico.uf2` to
the `RP2350` drive. The IP address shows up on the USB console
(`minicom -D /dev/ttyACM0`). Then:

```sh
cd dyno
PICO_IP=<ip> ./dyno.sh simpla    # make + upload (name without extension)
```

After every firmware rebuild, flash the firmware again and rebuild the uploaded
code (`make` tracks `build/dynapico.elf`). The device also rejects code built
against a different firmware build.

An uploaded program:

```c
#include "dyno.h"

MAIN_ENTRY(mem)                          /* mem: uint32_t[100], survives uploads */
{
    loguj_n("Run count: ", (int)++mem[0]);
    return (int)mem[0];
}
```

Sample programs:

| Program | Purpose |
|---|---|
| `simpla` | Counter in persistent memory + LED blinking |
| `blikej` | Infinite loop on core 1 (blinks the LED); the next upload stops it |
| `pad` | Deliberate HardFault on core 1; the server must keep working |
| `reset` | Restart. Hold BOOTSEL during the restart and the device stays in the USB bootloader, so there is no need to unplug the cable |
| `bootsel` | Always reboot into the USB bootloader |

## Limitations and risks

- **No security.** Port 3333 has no authentication or encryption: anyone on the
  network can run arbitrary code with full privileges on the device.
- Isolation is limited to the core level. A wild write (lwIP structures,
  peripherals) can still damage core 0. Resetting core 1 does not release
  locks the program held (e.g. the mutex inside `printf`); a restart helps.
- No standard library (`-nostdlib`). Code for which the compiler emits
  `memset`/`memcpy` (e.g. `char b[300] = {0}`) fails at link time.
  Code is limited to 16 kB.
- Globals do not survive the next upload; persistent state belongs in the
  `memory` array passed to `main`.
- `sig` is a heuristic based on addresses, not a hash of the firmware contents.
- The WiFi password ends up in `build/` as plain text.

## Ideas

- MPU: core 1 sees only `exec_buf`, `memory` and its stack.
- A HardFault handler on core 1 that reports the crash.
- Authentication or signing of the uploaded code.
- More API: I2C/SPI/PWM, UDP, `memcpy`/`memset` exported from the firmware.

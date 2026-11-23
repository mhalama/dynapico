# dynapico – nahrávání kódu po TCP do Pico 2 W (RP2350)

Port dřívějšího projektu pro ESP32-C6 (adresář `esp32c6`, už smazán) na Raspberry Pi Pico 2 W
v čistém C nad pico-sdk. Firmware běží, po TCP :3333 přijme surovou binárku,
zkopíruje ji do RAM a zavolá. Nahrávaný kód se linkuje na pevnou adresu
bufferu a funkce firmwaru volá přes absolutní adresy. Cyklus „úprava → spuštění"
trvá zlomek sekundy, firmware se nepřeflashovává.

**Stav: funguje na zařízení** (SDK 2.3.1, Pico 2 W).

## 1. Toolchain

| Položka | Použito |
|---|---|
| `arm-none-eabi-gcc` 13.2.1 (apt `gcc-arm-none-eabi`) | systémový; `-mcpu=cortex-m33` |
| pico-sdk **2.3.1** vč. submodulů `cyw43-driver`, `lwip`, `tinyusb` | `~/pico/pico-sdk` (`PICO_SDK_PATH`) |
| picotool | SDK ho samo stáhne a přeloží při prvním buildu |
| cmake, ninja, `libusb-1.0-0-dev`, `pkg-config` | systémové |

`~/pico/gcc-arm-none-eabi-10.3` se nepoužívá (nezná Cortex-M33).

## 2. Použití

```sh
# .env (není v gitu):  WIFI_SSID=...  WIFI_PASSWORD=...
./build.sh                       # firmware -> build/dynapico.uf2
# BOOTSEL + zkopírovat dynapico.uf2 na disk RP2350; IP je vidět v USB konzoli
cd dyno
PICO_IP=<ip> ./dyno.sh simpla    # make + odeslání, vypíše odpověď zařízení
```

Po každém novém buildu firmwaru je nutné firmware nahrát do zařízení a
nahrávaný kód přeložit znovu (`make` to hlídá závislostí na `build/dynapico.elf`;
zařízení navíc odmítne kód přeložený proti jinému buildu).

## 3. Struktura

| Cesta | Účel |
|---|---|
| `CMakeLists.txt`, `build.sh` | build firmwaru, WiFi údaje z `.env` |
| `src/main.c` | WiFi, start serveru, hlavní smyčka (tady se kód spouští) |
| `src/tcp_server.c/.h` | lwIP raw TCP server: callbacky jen sbírají bajty |
| `src/dyno.c/.h` | kontrola hlavičky, kopie do `exec_buf`, volání, implementace API |
| `src/dyno_api.h` | API firmwaru dostupné nahranému kódu (`DYNO_EXPORT`) |
| `src/lwipopts.h` | konfigurace lwIP (NO_SYS) |
| `dyno/prep.c` | PC nástroj: z `build/dynapico.elf` generuje `dyno_ptr.h` a `dyno_build.h` |
| `dyno/dynosend.c` | PC klient: přidá hlavičku, odešle, vypíše odpověď |
| `dyno/linker.ld`, `Makefile`, `dyno.sh` | překlad `.c` → `.bin` na adresu `exec_buf` |
| `dyno/simpla.c` | ukázkový program |

## 4. Rozdíly oproti ESP32-C6

| Téma | ESP32-C6 | RP2350 (Pico 2 W) |
|---|---|---|
| Jádro | RISC-V | Cortex-M33, Thumb-2 (`-mcpu=cortex-m33 -mthumb`) |
| Spustitelná RAM | IRAM + vypnout PMP split | SRAM je spustitelná, buffer je obyčejné pole |
| Adresa funkce | `nm` adresa | `nm` bit 0 (Thumb) **nevypisuje**; `prep` ho doplní, `exec_buf` se volá s `\|1` |
| Cache | – | pro SRAM žádná; stačí `dsb; isb` |
| TCP | sockety v FreeRTOS tasku | lwIP raw API, callbacky v kontextu lwIP |
| Kde běží kód | v tasku serveru | v hlavní smyčce, ne v lwIP kontextu |
| Ochrana exportů před `--gc-sections` | atribut `retain` | gcc 13.2 pro ARM `retain` **nezná**; CMake čte `DYNO_EXPORT(...)` z `dyno_api.h` a předá linkeru `--undefined=` |
| Linker | `main == DYNO_BASE` | `(main & ~1) == DYNO_BASE` |
| První nahrání | `idf.py flash` | BOOTSEL + UF2, dál po WiFi |

## 5. Protokol

Klient pošle `[hlavička 16 B | kód]` a ukončí odesílání (FIN). Hlavička
(little endian): `magic "DYNO"`, `len`, `base` (adresa `exec_buf`),
`sig` (`exec_buf ^ dyno_run`). Firmware zkontroluje magic, délku, `base`
a `sig` (shoda s buildem firmwaru), kód spustí a pošle zpět text
(výstup `loguj*` a návratovou hodnotu), pak spojení zavře.

## 6. Omezení a rizika

- Žádná autentizace ani šifrování: kdo je v síti, spustí na zařízení libovolný kód.
- Žádná izolace, kód běží v hlavní smyčce. Nekonečná smyčka zablokuje příjem
  dalšího kódu (WiFi dál běží z IRQ) → nutný reset.
- Žádná libc (`-nostdlib`). Kód, ze kterého překladač vygeneruje `memset`/`memcpy`
  (např. `char b[300] = {0}`), skončí chybou linkeru. Překročení `exec_buf`
  (16 kB) také.
- Globální proměnné nepřežijí další nahrání; trvalý stav je v poli `memory[100]`.
- `sig` je heuristika (adresy), ne hash obsahu firmwaru.
- WiFi heslo se přes `-D` dostane do `build/` jako text.

## 7. Možné další kroky

- Spouštění na jádře 1 + watchdog / možnost kód přerušit.
- Autentizace (sdílený klíč / podpis) nahrávaného kódu.
- Další API: I2C/SPI/PWM, UDP, `memcpy`/`memset` exportované z firmwaru.

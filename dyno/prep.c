/*
    Pouziti: prep ../src/dyno_api.h        -> dyno_ptr.h (#define nazev ((typeof(nazev) *)0xadresa))
             prep --build                  -> dyno_build.h (DYNO_BUILD_BASE, DYNO_BUILD_SIG pro dynosend)
    Adresy cte z ../build/dynapico.elf pomoci arm-none-eabi-nm (u Thumb funkci doplni bit 0).
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ELF "../build/dynapico.elf"

static FILE *open_nm(void)
{
    FILE *nm = popen("arm-none-eabi-nm -n " ELF, "r");
    if (!nm) perror("popen");
    return nm;
}

/* Adresa symbolu; u kodu (T/t) se nastavi bit 0 - nm ho u Thumb funkci nevypisuje, ale volani ho vyzaduje. */
static bool lookup(const char *want, unsigned long *out)
{
    FILE *nm = open_nm();
    if (!nm) return false;
    char line[512], name[256], type;
    unsigned long addr;
    bool found = false;
    while (!found && fgets(line, sizeof line, nm))
        if (sscanf(line, "%lx %c %255s", &addr, &type, name) == 3 && !strcmp(name, want)) {
            *out = (type == 'T' || type == 't') ? addr | 1 : addr;
            found = true;
        }
    pclose(nm);
    return found;
}

static int build_header(void)
{
    unsigned long base, runner;
    if (!lookup("exec_buf", &base) || !lookup("dyno_run", &runner)) {
        fprintf(stderr, "V " ELF " chybi exec_buf/dyno_run - je firmware prelozeny?\n");
        return 1;
    }
    printf("#pragma once\n#define DYNO_BUILD_BASE 0x%lxu\n#define DYNO_BUILD_SIG 0x%lxu\n",
           base, base ^ runner);
    return 0;
}

static int ptr_header(const char *api)
{
    FILE *f = fopen(api, "r");
    if (!f) { perror(api); return 1; }

    printf("#pragma once\n\n");
    char line[256];
    int count = 0;
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "#define")) continue;
        char *p = strncmp(line, "DYNO_EXPORT(", 12) == 0 ? line : NULL;   /* jen na zacatku radku (ne v komentari) */
        char *l = p ? strchr(p, '(') : NULL;
        char *r = l ? strchr(l, ')') : NULL;
        if (!r || r == l + 1) continue;
        *r = 0;
        unsigned long addr;
        if (!lookup(l + 1, &addr)) {
            fprintf(stderr, "Symbol %s nenalezen v " ELF "\n", l + 1);
            fclose(f);
            return 1;
        }
        printf("#define %s ((typeof(%s) *)0x%lx)\n", l + 1, l + 1, addr);
        count++;
    }
    fclose(f);
    if (!count) { fprintf(stderr, "Zadne DYNO_EXPORT(...)\n"); return 1; }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--build")) return build_header();
    if (argc == 2) return ptr_header(argv[1]);
    fprintf(stderr, "Pouziti: %s dyno_api.h | --build\n", argv[0]);
    return 1;
}

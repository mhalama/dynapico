/*
    Usage: prep ../src/dyno_api.h    -> dyno_ptr.h (#define name ((typeof(name) *)0xaddress))
           prep --build              -> dyno_build.h (DYNO_BUILD_BASE, DYNO_BUILD_SIG for dynosend)
    Addresses are read from ../build/dynapico.elf with arm-none-eabi-nm (bit 0 is added for Thumb functions).
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

/* Symbol address; for code (T/t) bit 0 is set - nm does not print it for Thumb functions, but calls require it. */
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
        fprintf(stderr, "exec_buf/dyno_run missing in " ELF " - is the firmware built?\n");
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
        char *p = strncmp(line, "DYNO_EXPORT(", 12) == 0 ? line : NULL;   /* only at the start of a line (not in a comment) */
        char *l = p ? strchr(p, '(') : NULL;
        char *r = l ? strchr(l, ')') : NULL;
        if (!r || r == l + 1) continue;
        *r = 0;
        unsigned long addr;
        if (!lookup(l + 1, &addr)) {
            fprintf(stderr, "Symbol %s not found in " ELF "\n", l + 1);
            fclose(f);
            return 1;
        }
        printf("#define %s ((typeof(%s) *)0x%lx)\n", l + 1, l + 1, addr);
        count++;
    }
    fclose(f);
    if (!count) { fprintf(stderr, "No DYNO_EXPORT(...) found\n"); return 1; }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "--build")) return build_header();
    if (argc == 2) return ptr_header(argv[1]);
    fprintf(stderr, "Usage: %s dyno_api.h | --build\n", argv[0]);
    return 1;
}

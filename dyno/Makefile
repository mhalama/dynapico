CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-ld
NM      = arm-none-eabi-nm
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump

# Cortex-M33, Thumb, zadne -fpic: kod se linkuje na pevnou adresu exec_buf
ARCH    = -mcpu=cortex-m33 -mthumb -mfloat-abi=soft
CFLAGS  = -Os $(ARCH) -ffreestanding -nostdlib -fno-builtin -fno-common \
          -fno-asynchronous-unwind-tables -fno-unwind-tables -Wall -Wextra

FIRMWARE_ELF = ../build/dynapico.elf
EXEC_BUF := $(shell $(NM) -S $(FIRMWARE_ELF) 2>/dev/null | awk '$$4 == "exec_buf" { print "0x" $$1, "0x" $$2 }')
DYNO_BASE := $(word 1,$(EXEC_BUF))
DYNO_SIZE := $(word 2,$(EXEC_BUF))

LDFLAGS = -T linker.ld --defsym=DYNO_BASE=$(DYNO_BASE) --defsym=DYNO_SIZE=$(DYNO_SIZE) \
          --orphan-handling=error --fatal-warnings --no-warn-rwx-segments
LIBGCC := $(shell $(CC) $(ARCH) -print-libgcc-file-name)

.DELETE_ON_ERROR:
.PRECIOUS: %.o %.elf

all: dynosend simpla.bin
	@true

prep: prep.c
	gcc -Wall -Wextra -o $@ $<

dyno_ptr.h: prep ../src/dyno_api.h $(FIRMWARE_ELF)
	./prep ../src/dyno_api.h >$@

dyno_build.h: prep $(FIRMWARE_ELF)
	./prep --build >$@

dynosend: dynosend.c dyno_build.h
	gcc -Wall -Wextra -o $@ $<

%.o: %.c dyno.h dyno_ptr.h
	$(CC) -c $(CFLAGS) $< -o $@

%.elf: %.o linker.ld $(FIRMWARE_ELF)
	@test -n "$(DYNO_SIZE)" || { echo "V $(FIRMWARE_ELF) nenalezen exec_buf - je firmware prelozeny?" >&2; exit 1; }
	$(LD) $(LDFLAGS) $< $(LIBGCC) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -d $< >$@.dump

clean:
	rm -f prep dynosend dyno_ptr.h dyno_build.h *.dump *.elf *.o *.bin

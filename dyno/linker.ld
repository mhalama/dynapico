/*
    The code is linked directly at the address of exec_buf in the firmware
    (DYNO_BASE, DYNO_SIZE come from the Makefile).
    Cortex-M33 / Thumb: the symbol main has bit 0 = 1, hence the (main & ~1) comparison.
*/
ENTRY(main)

MEMORY
{
  EXEC (rwx) : ORIGIN = DYNO_BASE, LENGTH = DYNO_SIZE
}

SECTIONS
{
  .image :
  {
    KEEP(*(.start))                       /* entry point must be at offset 0 */
    *(.text .text.*)
    *(.rodata .rodata.* .srodata .srodata.*)
    *(.data .data.*)
    *(.bss .bss.* COMMON)                 /* .bss is part of the binary as zeros - the firmware clears nothing */
  } > EXEC

  /DISCARD/ :
  {
    *(.comment) *(.note .note.*) *(.ARM.attributes)
    *(.ARM.exidx .ARM.exidx.* .ARM.extab .ARM.extab.*)
    *(.eh_frame .eh_frame_hdr)
    *(.glue_7 .glue_7t .vfp11_veneer .v4_bx .iplt .rel.iplt .igot.plt)   /* empty linker-generated sections */
  }
}

ASSERT((main & ~1) == DYNO_BASE, "Entry function is not at the start of the image - use MAIN_ENTRY()")

/*
    Kod se linkuje primo na adresu exec_buf ve firmwaru (DYNO_BASE, DYNO_SIZE z Makefile).
    Cortex-M33 / Thumb: symbol main ma bit 0 = 1, proto se porovnava (main & ~1).
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
    KEEP(*(.start))                       /* vstupni bod na offsetu 0 */
    *(.text .text.*)
    *(.rodata .rodata.* .srodata .srodata.*)
    *(.data .data.*)
    *(.bss .bss.* COMMON)                 /* .bss je v binarce jako nuly - firmware nic nenuluje */
  } > EXEC

  /DISCARD/ :
  {
    *(.comment) *(.note .note.*) *(.ARM.attributes)
    *(.ARM.exidx .ARM.exidx.* .ARM.extab .ARM.extab.*)
    *(.eh_frame .eh_frame_hdr)
    *(.glue_7 .glue_7t .vfp11_veneer .v4_bx .iplt .rel.iplt .igot.plt)   /* prazdne sekce linkeru */
  }
}

ASSERT((main & ~1) == DYNO_BASE, "Vstupni funkce neni na zacatku obrazu - pouzij MAIN_ENTRY()")

OUTPUT_ARCH(i386);
SECTIONS
{
    . = 0x30000;
    .entry : {*(.entry) }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
}
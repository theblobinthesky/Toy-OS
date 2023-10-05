OUTPUT_ARCH(i386);
SECTIONS
{
    . = 0x10000;
    .text : { *(.text*) }
    .data : { *(.data*) }
}
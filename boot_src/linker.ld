OUTPUT_ARCH(i386);
SECTIONS
{
    . = 0x7c00;
    .boot1 : { *(.boot1*) }
    . = 0x7e00;
    .boot2 : { *(.boot2*) }
}
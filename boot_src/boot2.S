.code16
.section .boot2, "ax"

call clear_vga_buffer

lea boot2_happy_noises_str, %bx
call print_string

jmp .

#include "string.S"

boot2_happy_noises_str: .asciz "Happy noises from stage 2!"
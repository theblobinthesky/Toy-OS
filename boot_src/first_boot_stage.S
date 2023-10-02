.code16
.section .boot, "ax"

.set vga_text_buffer, 0xb800
.set vga_text_buffer_end_offset, 0x7fff - 1
.set load_address, 0x7c00

# print some text out
# vga buffer is at 0xb8000 == (0xb800 << 4)
mov $vga_text_buffer, %ax
mov %ax, %es

# clear vga buffer
mov $0, %bx

.clearloop:
cmp $vga_text_buffer_end_offset, %bx
jge .clearexit
movw $0x1F00, %es:(%bx) # blue background, white foreground, clear codepoint
add $2, %bx
jmp .clearloop
.clearexit:

# write text to buffer
lea text, %bx
mov $0, %cx

.textloop:
mov %cx, %si
add %bx, %si    # base + index * 1
movb 0(%si), %dl
cmpb $0, %dl
je .textexit

mov %cx, %si
shl $1, %si     # index * 2
movb %dl, %es:(%si)
add $1, %cx
jmp .textloop
.textexit:

# setup segment register to 0x0
mov $0, %ax
mov %ax, %fs 
mov %ax, %gs 
mov %ax, %cs 
mov %ax, %ss 
mov %ax, %ds 
mov %ax, %es # this also resets the print function setup

# setup stack to grow down from the load address since the program grows upwards
# TODO: check if this interferes with the bios so be warned!
mov $load_address, %bp
mov %bp, %sp

# enable interrupts just in case
sti

hlt

text: .asciz "We are running in real mode!"

# since this is a floppy disk there is no need for a partition table
# not enough space anyways so it doesn't matter

.org 510
.byte 0x55
.byte 0xaa
.code16
.section .boot, "ax"

.set vga_text_buffer, 0xb800
.set vga_text_buffer_end_offset, 0x7fff - 1

# print some text out
# vga buffer is at 0xb8000 == (0xb800 << 4)
mov $vga_text_buffer, %ax
mov %ax, %es

# clear vga buffer
mov $0, %bx

.clearloop:
cmp $vga_text_buffer_end_offset, %bx
jge .clearexit
movb $0, %es:(%bx)
add $2, %bx
jmp .clearloop
.clearexit:

# write text to buffer
lea text, %bx
mov $0, %cx

.loop:
mov %cx, %si
add %bx, %si    # base + index * 1
movb 0(%si), %dl
cmpb $0, %dl
je .exit

mov %cx, %si
shl $1, %si     # index * 2
movb %dl, %es:(%si)
add $1, %cx
jmp .loop

.exit:
hlt

# TODO:
# initialize the gdt
# initialize the idt
# switch to protected mode

text: .asciz "We are running in real mode!"

.org 510
.byte 0x55
.byte 0xaa
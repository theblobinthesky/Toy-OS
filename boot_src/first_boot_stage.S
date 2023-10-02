.code16
.section .boot, "ax"

.set vga_text_buffer, 0xb800
.set vga_text_buffer_end_offset, 0x7fff - 1
.set load_address, 0x7c00

# the stack pointer is not setup yet
# so don't get interrupted yet
cli

# save the drive we booted from
movb boot_drive, %dl

# setup segment registers, not sure why you would do that
# this is taken from here: http://www.independent-software.com/operating-system-development-first-and-second-stage-bootloaders.html
# TODO: investigate if this is necessary
# It is clear that the cs register shouldn't be changed (like set to 0x0) though!
mov %cs, %ax 
mov %ax, %fs 
mov %ax, %gs 
mov %ax, %ss 
mov %ax, %ds 
mov %ax, %es

# setup stack to grow down from the load address since the program grows upwards
# TODO: check if this interferes with the bios so be warned!
mov $load_address, %bp
mov %bp, %sp

# reenable interrupts
sti

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

# reset the disk to ensure all reads and writes that are still pending
# are terminated and the controller is in a known state
movb boot_drive, %dl
movb $0, %ah # reset the disk
int $0x13
jc bootfailure

# successful happy noises!
lea happy_noises_str, %bx
call print_string

hlt


print_string:
mov $vga_text_buffer, %ax
mov %ax, %es

# write text to buffer
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
ret

bootfailure:
lea boot_failure_str, %bx
call print_string

hlt

happy_noises_str: .asciz "We are running in real mode!"
boot_failure_str: .asciz "Boot loader stage one failure!"

boot_drive: .byte 0

# since this is a floppy disk there is no need for a partition table
# not enough space anyways so it doesn't matter

.org 510
.byte 0x55
.byte 0xaa
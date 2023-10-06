
.code16
.section .boot1, "ax"
.set boot1_load_address, 0x7c00
.set boot2_load_address, (boot1_load_address + 0x200)
#include "constants.S"

jmp main # near jump
nop # pad to 3 bytes

#include "bootsector.S"
#include "drive.S"

main:
# the stack pointer is not setup yet
# so don't get interrupted yet
cli

# save the drive we booted from
movb %dl, boot_drive

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
mov $boot1_load_address, %bp
mov %bp, %sp

# reenable interrupts
sti

call clear_vga_buffer

# reset the disk to ensure all reads and writes that are still pending
# are terminated and the controller is in a known state
movb boot_drive, %dl
movb $0, %ah # reset the disk
int $0x13
jc boot_failure

# load boot stage 2:
find_file_in_root_directory root_dir_segment, root_dir_offset, boot2_file_name
push %ax # push the logical cluster
read_fat_sectors fat_segment
pop %ax # pop the logical cluster
read_file fat_segment, %ax, 0x0, boot2_load_address 

ljmpw $0, $boot2_load_address

boot_failure:
    lea boot_failure_str, %bx
    call print_string

    jmp .

boot_failure_str: .asciz "Boot loader stage one failed!"
boot2_file_name: .asciz "BOOT2   BIN"
boot_drive: .byte 0

#include "string.S"

# since this is a floppy disk there is no need for a partition table
# not enough space anyways so it doesn't matter

.org 510
.byte 0x55
.byte 0xaa
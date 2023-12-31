.code16
.section .boot2, "ax"
#include "constants.S"

jmp main # near jump
nop # pad to 3 bytes

#include "bootsector.S"
#include "drive.S"

main:
call clear_vga_buffer

lea boot2_happy_noises_str, %bx
call print_string

# load kernel code:
find_file_in_root_directory root_dir_segment, root_dir_offset, kernel_file_name
push %ax # push the logical cluster
read_fat_sectors fat_segment# the fat has already been read by boot stage 1
pop %ax # pop the logical cluster
read_file fat_segment, %ax, kernel_segment, 0x0

# switch to protected mode:
# no exceptions or interrupts must be generated during mode switching
cli

# enable the a20 gate
call check_a20      # check
cmp $0, %ax
jne .enabled_a20

call enable_a20     # try to enable

call check_a20      # check again
cmp $0, %ax
jne .enabled_a20

jmp boot_failure    # fail

.enabled_a20:

# # detect available memory
# call detect_memory

# # load the whole program into memory starting at 1MB
# call load_main

# load gdtr register with gdt to prepare for protected mode
lgdt (gdt_descriptor)

# set protected mode flag
mov %cr0, %eax
or $1, %ax # set PE (protected mode) flag
mov %eax, %cr0

# serialize the processor using a far jump instruction to prevent pipeline inconsistencies
ljmp $kernel_code_segment_offset, $protected_mode_entry # also sets the code segment cs to the right descriptor

check_a20:
    pushw %ds
    pushw %es
    pushw %si
    pushw %di
    
    mov $0xffff, %ax
    mov %ax, %es
    mov $0x0400, %si

    mov $0x0000, %ax
    mov %ax, %ds
    mov $0x0410, %di

    movb %es:0(%si), %dl
    movb %ds:0(%di), %dh

    movb $0x00, %es:0(%si)
    movb $0xff, %ds:0(%di)
    movb %es:0(%si), %al
    cmp $0x00, %al

    mov $0, %ax
    je .a20_enabled
    jmp .a20_exit
    
    .a20_enabled:
        mov $1, %ax
    .a20_exit:
        movb %dl, %es:0(%si)
        movb %dh, %ds:0(%di)

        popw %di
        popw %si
        popw %es
        popw %ds
        ret

# this method is not sufficient on many old systems
# see: https://www.win.tue.nl/~aeb/linux/kbd/A20.html
# enable a20 using the fast a20 gate
enable_a20:
    in $0x92, %al
    test $2, %al
    jnz .enable_a20_exit
    or $2, %al
    out %al, $0x92

    .enable_a20_exit:
        ret

boot_failure:
    lea boot_failure_str, %bx
    call print_string

    jmp .

#include "string.S"

boot_failure_str: .asciz "Boot loader stage two failed!"
kernel_file_name: .asciz "KERNEL  BIN"
boot_drive: .byte 0

# 32bit territory starts from here:
.code32
protected_mode_entry:
# set the segment registers to the right descriptor
mov $kernel_data_segment_offset, %ax
mov %ax, %es
mov %ax, %ss
mov %ax, %ds
mov %ax, %fs
mov %ax, %gs

mov $0xffffffff, %eax
movb $'R', 0xb8000

ljmp $kernel_code_segment_offset, $kernel_flat_address

boot2_happy_noises_str: .asciz "Happy noises from stage 2!"

.align 8 # NOTE: align might be unnecessary
gdt:
.gdt_zero_segment_entry:
    # entry 0: zero entry
    .int 0x0
    .int 0x0

.gdt_kernel_code_segment_entry:
    # entry 1: kernel code segment (see gdt_generator.py)
    .int 0x0000ffff
    .int 0x00cf9b00

.gdt_kernel_data_segment_entry:
    # entry 2: kernel data segment (see gdt_generator.py)
    .int 0x0000ffff
    .int 0x00cf9300

.gdt_user_code_segment_entry:
    # entry 3: user code segment (see gdt_generator.py)
    .int 0x0000ffff
    .int 0x00cffb00

.gdt_user_data_segment_entry:
    # entry 4: user data segment (see gdt_generator.py)
    .int 0x0000ffff
    .int 0x00cff300
gdt_end:

.set kernel_code_segment_offset, (.gdt_kernel_code_segment_entry - gdt)
.set kernel_data_segment_offset, (.gdt_kernel_data_segment_entry - gdt) 
.set user_code_segment_offset, (.gdt_user_code_segment_entry - gdt)
.set user_data_segment_offset, (.gdt_user_data_segment_entry - gdt) 

.align 8 # nocheckin: align might be unnecessary
gdt_descriptor:
    .gdt_descriptor_limit:  .word gdt_end - gdt # nocheckin
    .gdt_descriptor_offset: .int gdt
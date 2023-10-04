.code16
.section .boot, "ax"
.set vga_text_buffer, 0xb800
.set vga_text_buffer_end_offset, 0x7fff - 1
.set load_address, 0x7c00

jmp main # near jump
nop # pad to 3 bytes

bootsector:
    .org 3
    .boot_oem: .ascii "Rick Ast" # TODO: this might be too long
    .org 11
    .sector_size:           .word 512
    .sectors_per_cluster:   .byte 1
    .num_reserved_sectors:  .word 1 # at least 1 and starting from lba sector 0 (TODO: does this mean "exclude the mbr sector")
    .num_fats:              .byte 2 # two fats for redundancy?
    .max_num_root_entries:  .word 512 # todo: is that right? 
    .num_sectors:           .word 0 # todo: ?
    .media_desc:            .byte 0xF0 # DOS Version 3.3, 3 1/2 floppy 1.44MB
    .sectors_per_fat:       .word 0 # todo: ?
    .sectors_per_head:      .word 9 # just use these values dude
    .heads_per_cylinder:    .word 2 # just use these values dude
    .int 0 # ignored for fat12, number of hidden sectors
    .int 0 # only for fat32 and ignored for fat12, total sector count
    .extended_boot_sig:     .byte 0x29 # indicates another three fields follow
    .volume_id:             .ascii "seri" # a serial number
    .volume_label:          .ascii "NO NAME    " # 11 bytes
    .fs_type:               .ascii "FAT12   "

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
mov $load_address, %bp
mov %bp, %sp

# reenable interrupts
sti

call clear_vga_buffer

# reset the disk to ensure all reads and writes that are still pending
# are terminated and the controller is in a known state
movb boot_drive, %dl
movb $0, %ah # reset the disk
int $0x13
jc bootfailure

.macro read_sector_test index
pushw $\index # lba
pushw $1 # number of sectors
pushw $0x4000 # es
pushw $0x0000 # offset
call read_sector
add $8, %sp
movw %es:0, %ax
cmp $\index, %ax
jne bootfailure
.endm

read_sector_test 1
read_sector_test 2
read_sector_test 3
read_sector_test 4


# successful happy noises!
lea happy_noises_str, %bx
call print_string

jmp .

# Push the lba address (0-indexed), 
# then the number of sectors to-be-read,
# then the segment register value
# and then the memory offset onto the stack.
# NOTE: segment:offset + sector_size*number_of_sectors should not cross a 64kbit e.g. 0x10000==256^2 boundary.
# For example: segment:offset==0x4ff00 and one sector of size 512, e.g. 0x200 would result in segment:offset+0x200==0x50100 which would be invalid. 
read_sector:
    push %bp
    mov %sp, %bp

    mov 10(%bp), %ax # lba address
    mov 8(%bp), %di # number of sectors
    mov 6(%bp), %es # segment register value
    mov 4(%bp), %bx # memory offset

    # convert lba (logical block addressing) to chs (cylinder head sector) addressing:
    # for the conversion from an address tuple (cylinder, head, sector) 
    # with H heads per cylinder and S sectors per head to lbs use
    # LBA = (cylinder * H + head) * S + sector - 1 and solve for (cylinder, head, sector). 
    # That gives:   sector = (LBA % S) + 1 
    #               head = (LBA / S) % H 
    #               cylinder = (LBA / S) / H

    # Things should end up like to this:
    #   ah = 0x2
    #   not checking for cylinder boundary: al = number of sectors (0 is illegal) -- cannot cross ES page boundary, or a cylinder boundary, and must be < 128
    #   ch = cylinder & 0xff
    #   cl = sector | ((cylinder >> 2) & 0xC0);
    #   dh = head
    #   dl = drive number
    #   es:bx = buffer address pointer

    # compute sector
    xor %dx, %dx
    divw .sectors_per_head # dx:ax / .sectors_per_head
    mov %dl, %cl
    add $1, %cl
    # changed state: cl == sector, ax == LBA / S

    # compute head and cylinder
    xor %dx, %dx
    divw .heads_per_cylinder
    # changed state: dx == (LBA / S) % H, ax == (LBA / S) / H 

    mov %al, %ch
    shr $2, %ax
    and $0xC0, %ax  
    or %al, %cl
    # changed state: ch == cylinder & 0xff, cl == sector | ((cylinder >> 2) & 0xC0)

    xchg %dl, %dh   # pre: dx == (0, head number), instruction, post: dx == (head number, 0) and dh = head number
    xor %dl, %dl    # NOTE: looks nice while debugging
    # changed state: dh == head

    mov %di, %ax
    # changed state: al == number of sectors

    movb $2, %ah    # select subfunction 2 "read sector"
    movb boot_drive, %dl  # read from **this** drive
    # read to memory at es:bx

    int $0x13
    jc bootfailure 

    leave
    ret

bootfailure:
    lea boot_failure_str, %bx
    call print_string

    jmp .

.macro mov_vga_text_buffer_to_es
    # vga buffer is at 0xb8000 == (0xb800 << 4)
    mov $vga_text_buffer, %ax
    mov %ax, %es
.endm

clear_vga_buffer:
    mov_vga_text_buffer_to_es

    mov $0, %bx # offset into vga buffer

    .clearloop:
    cmp $vga_text_buffer_end_offset, %bx
    jge .clearexit
    movw $0x1F00, %es:(%bx) # blue background, white foreground, clear codepoint
    add $2, %bx
    jmp .clearloop

    .clearexit:
    ret

print_string:
    mov_vga_text_buffer_to_es

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

happy_noises_str: .asciz "We are running in real mode!"
boot_failure_str: .asciz "Boot loader stage one failed!"

boot_drive: .byte 0

# since this is a floppy disk there is no need for a partition table
# not enough space anyways so it doesn't matter

.org 510
.byte 0x55
.byte 0xaa

# TODO: delete this later

# sector 1
.org (1*512)
.word 1

# sector 2
.org (2*512)
.word 2

# sector 3
.org (3*512)
.word 3

# sector 4
.org (4*512)
.word 4
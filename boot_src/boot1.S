.code16
.section .boot1, "ax"
.set boot1_load_address, 0x7c00
.set boot2_load_address, (boot1_load_address + 0x200)
.set root_directory_num_sectors, 14
.set directory_entries_per_sector, (512/32)

jmp main # near jump
nop # pad to 3 bytes

bootsector: # TODO: copy the real values over; these are not the real ones anymore
    .org 3
    boot_oem: .ascii "Rick Ast" # TODO: this might be too long
    sector_size:           .word 0x200
    sectors_per_cluster:   .byte 1
    num_reserved_sectors:  .word 1 # at least 1 and starting from lba sector 0 (TODO: does this mean "exclude the mbr sector")
    num_fats:              .byte 2 # two fats for redundancy
    max_num_root_entries:  .word (14*16) 
    num_sectors:           .word 0 # todo: ?
    media_desc:            .byte 0xF0 # DOS Version 3.3, 3 1/2 floppy 1.44MB
    sectors_per_fat:       .word 0 # todo: ?
    sectors_per_head:      .word 9 # just use these values dude
    heads_per_cylinder:    .word 2 # just use these values dude
    .int 0 # ignored for fat12, number of hidden sectors
    .int 0 # only for fat32 and ignored for fat12, total sector count
    .word 0 # ignored for fat12
    extended_boot_sig:     .byte 0x29 # indicates another three fields follow
    volume_id:             .ascii "seri" # a serial number
    volume_label:          .ascii "NO NAME    " # 11 bytes
    fs_type:               .ascii "FAT12   "

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
jc bootfailure

call find_file_in_root_directory
mov %ax, boot2_first_logical_cluster

call read_fat_sectors

call read_file

ljmpw $0, $boot2_load_address
jmp bootfailure # safeguard

# Push the lba address (0-indexed), 
# then the number of sectors to-be-read,
# then the segment register value
# and then the memory offset onto the stack.
# NOTE: segment:offset + sector_size*number_of_sectors should not cross a 64kbit e.g. 0x10000==256^2 boundary.
# For example: segment:offset==0x4ff00 and one sector of size 512, e.g. 0x200 would result in segment:offset+0x200==0x50100 which would be invalid. 
read_sector:
    push %bp
    mov %sp, %bp

    pusha # TODO: wasting stack space

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
    divw sectors_per_head # dx:ax / sectors_per_head
    mov %dl, %cl
    add $1, %cl
    # changed state: cl == sector, ax == LBA / S

    # compute head and cylinder
    xor %dx, %dx
    divw heads_per_cylinder
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

    popa # TODO: wasting stack space

    leave
    ret

.set r_es, 0x4000
.set r_of, 0x0000
find_file_in_root_directory:
    push %bp
    mov %sp, %bp

    # clear the direction flag to ensure forward comparison
    cld
    
    # load directory entry segment into es
    mov $r_es, %ax
    mov %ax, %es

    # compute the first sector in the root directory in ax:
    xor %ax, %ax
    movb num_fats, %al
    mulw sectors_per_fat
    addw num_reserved_sectors, %ax

    mov $root_directory_num_sectors, %bx
    .sector_loop:
        pushw %ax   # lba
        pushw $1    # number of sectors
        pushw $r_es # es
        pushw $r_of # offset
        call read_sector
        add $6, %sp # don't pop ax here
        # r_es:r_of contains the next sector

        mov $directory_entries_per_sector, %dx
        .entry_loop:
            # load directory entry into di
            mov %dx, %di
            dec %di
            shl $5, %di     # times 32
            add $r_of, %di  # + base offset
            mov %di, %ax # in case we return save it

            # load readme file name string into ds:si
            lea boot2_file_name, %si

            # strcmp es:di (the entry name) with ds:si (the name you're searching for)
            mov $11, %cx # len(filename + extension) == 1
            repz cmpsb
            je .found_file

            dec %dx
            jnz .entry_loop

        pop %ax
        inc %ax # increase lba for next time

        dec %bx
        jnz .sector_loop
    
    jmp bootfailure

    .found_file:
        # read first logical cluster into ax
        mov %ax, %bx # TODO: ugly copy
        movw %es:26(%bx), %ax

        leave
        ret

.set f_es, 0x5000
.set f_of, 0x0000
read_fat_sectors:
    movw num_reserved_sectors, %ax
    movw sectors_per_fat, %cx 
    
    pushw %ax   # lba
    pushw %cx   # number of sectors
    pushw $f_es # es
    pushw $f_of # offset
    call read_sector
    add $8, %sp

    ret

# TODO: ugly code!
read_file:
    mov $boot2_load_address, %di
    mov boot2_first_logical_cluster, %bx

    .loop:
        # load segment into memory:
        # compute physical lba from logical cluster/sector which are the same in fat12
        # TODO: maybe cluster and sector are not always the same?
        mov $(33-2), %si    # nocheckin: do the actual computation
        add %bx, %si        # nocheckin: do the actual computation

        # read sector from disk
        pushw %si   # lba
        pushw $1    # number of sectors
        pushw $0    # es
        pushw %di   # offset
        call read_sector
        add $8, %sp

        # advance to the next sector
        add sector_size, %di

        # access next sector:
        # calculate remainder of x1.5 that gets lost when bx is uneven
        mov %bx, %cx
        and $1, %cx
        shl $2, %cx

        # calculate memory offset to read into fat
        mov %bx, %ax
        shr $1, %ax
        add %ax, %bx

        # load fat segment register        
        mov $f_es, %ax
        mov %ax, %es

        # load word from fat, shift if bx was uneven and mask away unrelated bits
        movw %es:(%bx), %dx
        shr %cl, %dx
        and $0x0fff, %dx
        mov %dx, %bx

        # check for last cluster and jump out if necessary
        cmp $0xff8, %bx
        jge .exit

        jmp .loop

    .exit:
        ret 

bootfailure:
    lea boot_failure_str, %bx
    call print_string

    jmp .

#include "string.S"

boot_failure_str: .asciz "Boot loader stage one failed!"
boot2_file_name: .asciz "BOOT2   BIN" # the second boot stage
boot2_first_logical_cluster: .word 0
boot_drive: .byte 0

# since this is a floppy disk there is no need for a partition table
# not enough space anyways so it doesn't matter

.org 510
.byte 0x55
.byte 0xaa
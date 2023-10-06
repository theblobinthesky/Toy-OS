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
    jc boot_failure 

    popa # TODO: wasting stack space

    leave
    ret

.macro find_file_in_root_directory root_dir_seg, root_dir_offs, file_name
    push %bp
    mov %sp, %bp

    # clear the direction flag to ensure forward comparison
    cld
    
    # load directory entry segment into es
    mov $\root_dir_seg, %ax
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
        pushw $\root_dir_seg  # es
        pushw $\root_dir_offs # offset
        call read_sector
        add $6, %sp # don't pop ax here
        # root_dir_seg:root_dir_offs contains the next sector

        mov $directory_entries_per_sector, %dx
        .entry_loop:
            # load directory entry into di
            mov %dx, %di
            dec %di
            shl $5, %di     # times 32
            add $\root_dir_offs, %di # + base offset
            mov %di, %ax # in case we return save it

            # load readme file name string into ds:si
            lea \file_name, %si

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
    
    jmp boot_failure

    .found_file:
        # read first logical cluster into ax
        mov %ax, %bx # TODO: ugly copy
        movw %es:26(%bx), %ax

        leave
.endm

.macro read_fat_sectors fat_segment
    movw num_reserved_sectors, %ax
    movw sectors_per_fat, %cx 
    
    pushw %ax   # lba
    pushw %cx   # number of sectors
    pushw $\fat_segment # es
    pushw $0     # offset
    call read_sector
    add $8, %sp
.endm

.macro read_file fat_segment, fat_logical_cluster, memory_segment, memory_address
    mov $\memory_address, %di
    mov \fat_logical_cluster, %bx

    .loop:
        # load segment into memory:
        # compute physical lba from logical cluster/sector which are the same in fat12
        # TODO: maybe cluster and sector are not always the same?
        mov $(33-2), %si    # nocheckin: do the actual computation
        add %bx, %si        # nocheckin: do the actual computation

        # read sector from disk
        pushw %si   # lba
        pushw $1    # number of sectors
        pushw $\memory_segment # es
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
        mov $\fat_segment, %ax
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
 .endm
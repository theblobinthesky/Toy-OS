with open("bin/floppy.bin", "rb") as file:
    bytes = bytearray(file.read())

bootsector = bytes[3:61]
index = 0

def cut(count: int) -> bytearray:
    global index
    slice = bootsector[index:index+count]
    index += count
    return slice

def cut_string(count: int) -> str:
    slice = cut(count)
    return slice.decode("ascii")

def cut_int(count: int, isHex: bool = True) -> str:
    assert count == 1 or count == 2 or count == 4
    slice = cut(count)
    
    if count == 1:
        number = slice[0]
    elif count == 2:
        number = (slice[1] << 8) | slice[0]
    elif count == 4:
        number = (slice[3] << 24) | (slice[2] << 16) | (slice[1] << 8) | slice[0]

    if isHex: return hex(number)
    else: return str(number)

print(f"bootsector: # the values are up-to-date")
print(f"    .org 3")
print(f"    boot_oem:              .ascii \"{cut_string(8)}\"")
print(f"    sector_size:           .word {cut_int(2)}")
print(f"    sectors_per_cluster:   .byte {cut_int(1, isHex=False)}")
print(f"    num_reserved_sectors:  .word {cut_int(2, isHex=False)} # at least 1 and starting from lba sector 0 (exclude the mbr sector)")
print(f"    num_fats:              .byte {cut_int(1, isHex=False)} # two fats for redundancy")
print(f"    max_num_root_entries:  .word {cut_int(2, isHex=False)}")
print(f"    num_sectors:           .word {cut_int(2)}")
print(f"    media_desc:            .byte {cut_int(1)}")
print(f"    sectors_per_fat:       .word {cut_int(2, isHex=False)}")
print(f"    sectors_per_head:      .word {cut_int(2, isHex=False)}")
print(f"    heads_per_cylinder:    .word {cut_int(2, isHex=False)}")
print(f"    .int 0 # ignored for fat12, number of hidden sectors"); cut_int(4)
print(f"    .int 0 # only for fat32 and ignored for fat12, total sector count"); cut_int(4)
print(f"    .word 0 # ignored for fat12"); cut_int(2)
print(f"    extended_boot_sig:     .byte {cut_int(1)} # indicates another three fields follow")
print(f"    volume_id:             .int {cut_int(4)} # a serial number")
print(f"    volume_label:          .ascii \"{cut_string(11)}\" # 11 bytes")
print(f"    fs_type:               .ascii \"{cut_string(8)}\" # 8 bytes")
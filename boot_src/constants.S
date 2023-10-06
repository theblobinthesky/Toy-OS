.set root_directory_num_sectors, 14
.set directory_entries_per_sector, (512/32)
.set root_dir_segment, 0x1000
.set root_dir_offset, 0x0000
.set fat_segment, 0x2000
.set kernel_segment, 0x3000
.set kernel_flat_address, (kernel_segment << 4)
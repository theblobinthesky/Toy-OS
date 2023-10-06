def get_gdt_entry(executable: bool, kernel_ring: bool):
    base = 0x0
    limit = 0xfffff

    present_bit = 1 << 7
    dpl = 0 if kernel_ring else 3
    type_bit = 1 << 4       # make this a code or data segment
    executable_bit = (1 << 3) if executable else 0
    dc_bit = 0
    rw_bit = (1 << 1)
    accessed_bit = (1 << 0)
    access_byte = present_bit | dpl | type_bit | executable_bit | dc_bit | rw_bit | accessed_bit

    granularity_flag = (1 << 3) # limit refers to 4 KIB blocks. this is called page granularity
    db_flag = (1 << 2) # 32bit descriptor
    long_mode_flag = 0
    flags = granularity_flag | db_flag | long_mode_flag

    desc = 0x0
    desc |= (limit & 0xffff)
    desc |= (base & 0x0fff)
    desc |= (access_byte << 40)
    desc |= ((((limit & (~0xffff)) >> 16) & 0xf) << 48)
    desc |= (flags << 52)
    desc |= (((base & 0xf000) >> 12) << 56)

    return desc

executable_modes = [False, True]
kernel_ring_modes = [False, True]

for executable_mode in executable_modes:
    for kernel_ring_mode in kernel_ring_modes:
        entry = get_gdt_entry(executable=executable_mode, kernel_ring=kernel_ring_mode)
        entry_low = entry & 0xffffffff
        entry_high = (entry >> 32) & 0xffffffff
        print(f"{'code' if executable_mode else 'data'} segment, {'kernel' if kernel_ring_mode else 'userspace'} privilege: 0x{entry_low:08x} 0x{entry_high:08x}")
def get_gdt_entry(executable: bool, kernel_mode: bool):
    base = 0x0
    limit = 0xfffff

    present_bit = 1 << 7
    dpl = (0 if kernel_mode else 3) << 5
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

def get_low_u32(num):
    return num & 0xffffffff

def get_high_u32(num):
    return (num >> 32) & 0xffffffff

entries = [
    ("kernel code segment", True, True),
    ("kernel data segment", False, True),
    ("user code segment", True, False),
    ("user data segment", False, False),
]

for i, (name, executable, kernel_mode) in enumerate(entries):
    gdt_entry = get_gdt_entry(executable, kernel_mode)
    print(f".gdt_{name.replace(' ', '_')}_entry:")
    print(f"    # entry {i + 1}: {name} (see gdt_generator.py)")
    print(f"    .int 0x{get_low_u32(gdt_entry):08x}")
    print(f"    .int 0x{get_high_u32(gdt_entry):08x}")
    print()
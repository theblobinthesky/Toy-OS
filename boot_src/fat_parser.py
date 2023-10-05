import numpy as np

with open("bin/floppy.bin", "rb") as file:
    bytes = bytearray(file.read())
    
bytes = np.array(bytes, np.uint8)
fat = bytes[(1 * 512):]

def sec(index): return index * 512

index = 3
indices = []
while index < 0xff8:
    indices.append(index)
    offset = index * 1.5
    offset_down = int(offset)
    view = fat[offset_down:offset_down+2]
    number = view[0] | (view[1] << 8)
    number = number if (index % 2 == 0) else (number >> 4)
    number = number & 0xfff
    index = number

for index in indices:
    print(f"index: {hex(index)}")

    physical_sector = 33 + index - 2
    sector = bytes[sec(physical_sector):sec(physical_sector + 1)]
    sector = [chr(b) for b in sector]
    print(sector)
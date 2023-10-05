ME=Makefile
C_SRCS=$(wildcard boot_src/*.c src/*.c)
C_DISASM=$(patsubst %.c,%_disasm.S,$(C_SRCS))
ASM_SRCS=$(wildcard boot_src/*.S src/*.S)

C_OBJS=$(patsubst %.c,%_c.o,$(C_SRCS))
ASM_OBJS=$(patsubst %.S,%_S.o,$(ASM_SRCS))

FLOPPY_FILE=bin/floppy.bin
MBR_FILE=bin/mbr.bin

GCC_ARGS=-c -ffreestanding -m32

%_c.o: %.c $(ME)
	gcc $(GCC_ARGS) $< -o $@

%_S.o: %.S $(ME)
	gcc $(GCC_ARGS) $< -o $@

boot_src/boot.o: boot_src/first_boot_stage_S.o
	ld -T boot_src/boot.ld $< -o $@

$(MBR_FILE): boot_src/boot.o
	objcopy -O binary -j .boot $< $@

$(FLOPPY_FILE): $(MBR_FILE)
	dd if=/dev/zero of=$@ bs=1024 count=1440 status=none
	mkfs.fat -F 12 -s 1 $@
	
	mkdir -p temporary_mount
	sudo mount $@ temporary_mount -o loop
	sudo cp README.md temporary_mount
	sudo umount temporary_mount
	rm -rf temporary_mount

	# dd if=$< of=$@ bs=1 count=512 conv=notrunc status=none
	dd if=$< of=$@ bs=1 count=3 conv=notrunc status=none
	dd if=$< of=$@ bs=1 skip=60 seek=60 conv=notrunc status=none
	
all: $(FLOPPY_FILE)

clean: clean-disasm
	rm -f bin/*
	rm -f boot_src/*.o
	rm -f src/*.o

# --- Disassembly ---

%_disasm.S: %.c
	gcc -c -S -fverbose-asm -O1 $< -o $@

disasm: $(C_DISASM)

clean-disasm:
	rm -f src/*_disasm.S
	rm -f boot_src/*_disasm.S
	rm -f bochs.log
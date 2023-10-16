ME=Makefile
C_SRCS=$(wildcard src/*.c)
C_HEADERS=$(wildcard src/*.h)
ASM_SRCS=$(wildcard src/*.S)

C_DISASM=$(patsubst %.c,%_disasm.S,$(C_SRCS))

C_OBJS=$(patsubst %.c,%_c.o,$(C_SRCS))
ASM_OBJS=$(patsubst %.S,%_S.o,$(ASM_SRCS))

BOOT_OBJS=boot_src/boot1_S.o boot_src/boot2_S.o

FLOPPY_FILE=bin/floppy.bin
BOOTSECTOR_S_FILE=boot_src/bootsector.S
BOOT1_FILE=bin/boot1.bin
BOOT2_FILE=bin/boot2.bin
KERNEL_FILE=bin/kernel.bin

COMMON_GCC_ARGS=-c -ffreestanding -m32 --no-pic
ifeq ($(BUILD_TYPE),debug)
	GCC_ARGS = $(COMMON_GCC_ARGS) -g -Ddebug
else
	GCC_ARGS = $(COMMON_GCC_ARGS) -O3
endif

$(BOOTSECTOR_S_FILE): boot_src/bootsector_parser.py
	dd if=/dev/zero of=$(FLOPPY_FILE) bs=1024 count=1440 status=none
	mkfs.fat -F 12 -s 1 $(FLOPPY_FILE)
	python3 $< > $@

%_c.o: %.c $(ME) $(C_HEADERS)
	gcc $(GCC_ARGS) $< -o $@

%_S.o: %.S $(ME) $(C_HEADERS)
	gcc $(GCC_ARGS) $< -o $@

boot_src/boot.o: $(BOOTSECTOR_S_FILE) $(BOOT_OBJS)
	ld -T boot_src/linker.ld $(BOOT_OBJS) -o $@

$(BOOT1_FILE): boot_src/boot.o
	objcopy -O binary -j .boot1 $< $@

$(BOOT2_FILE): boot_src/boot.o
	objcopy -O binary -j .boot2 $< $@

src/kmain.o: $(ASM_OBJS) $(C_OBJS)
	ld -T src/linker.ld $^ -o $@

$(KERNEL_FILE): src/kmain.o
	objcopy -O binary $< $@

$(FLOPPY_FILE): $(BOOT1_FILE) $(BOOT2_FILE) $(KERNEL_FILE)	
	mkdir -p temporary_mount
	sudo mount $@ temporary_mount -o loop
	sudo cp $(BOOT2_FILE) temporary_mount
	sudo cp $(KERNEL_FILE) temporary_mount
	sudo umount temporary_mount
	rm -rf temporary_mount
	dd if=$< of=$@ bs=1 count=512 conv=notrunc status=none
	
# shortcuts
boot1: $(BOOT1_FILE)
boot2: $(BOOT2_FILE)
kernel: $(KERNEL_FILE)
all: $(FLOPPY_FILE)

clean: clean-disasm
	rm -f bin/*
	rm -f boot_src/*.o
	rm -f src/*.o

# disassembly

%_disasm.S: %.c
	gcc -S -fverbose-asm $(GCC_ARGS) $< -o $@

disasm: $(C_DISASM)

clean-disasm:
	rm -f boot_src/bootsector.S
	rm -f src/*_disasm.S
	rm -f boot_src/*_disasm.S
	rm -f bochs.log
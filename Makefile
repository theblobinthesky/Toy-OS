ME=Makefile
C_SRCS=$(wildcard boot_src/*.c src/*.c)
C_DISASM=$(patsubst %.c,%_disasm.S,$(C_SRCS))
ASM_SRCS=$(wildcard boot_src/*.S src/*.S)

C_OBJS=$(patsubst %.c,%_c.o,$(C_SRCS))
ASM_OBJS=$(patsubst %.S,%_S.o,$(ASM_SRCS))

GCC_ARGS=-c -ffreestanding -m32

bin/padder: padder.c $(ME)
	gcc padder.c -o bin/padder

%_c.o: %.c
	gcc $(GCC_ARGS) $< -o $@

%_S.o: %.S
	gcc $(GCC_ARGS) $< -o $@

boot_src/boot.o: boot_src/first_boot_stage_S.o
	ld -T boot_src/boot.ld $< -o $@

bin/floppy.bin: boot_src/boot.o
	objcopy -O binary -j .boot $< $@

all: bin/padder bin/floppy.bin
	./bin/padder bin/floppy.bin

run: all

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
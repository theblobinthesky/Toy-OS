.set vga_text_buffer, 0xb800
.set vga_text_buffer_end_offset, 0x7fff - 1

.macro mov_vga_text_buffer_to_es
    # vga buffer is at 0xb8000 == (0xb800 << 4)
    mov $vga_text_buffer, %ax
    mov %ax, %es
.endm

clear_vga_buffer:
    mov_vga_text_buffer_to_es

    mov $0, %bx # offset into vga buffer

    .clearloop:
        cmp $vga_text_buffer_end_offset, %bx
        jge .clearexit
        movw $0x1F00, %es:(%bx) # blue background, white foreground, clear codepoint
        add $2, %bx
        jmp .clearloop

    .clearexit:
    ret

# put the memory offet into %bx
print_string:
    mov_vga_text_buffer_to_es

    # write text to buffer
    mov $0, %cx

    .textloop:
        mov %cx, %si
        add %bx, %si    # base + index * 1
        movb 0(%si), %dl
        cmpb $0, %dl
        je .textexit

        mov %cx, %si
        shl $1, %si     # index * 2
        movb %dl, %es:(%si)
        add $1, %cx
        jmp .textloop
    .textexit:

    ret
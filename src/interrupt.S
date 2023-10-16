.extern intr_handler_common

.macro no_errc_handler index name
.global intr_handler_\name
intr_handler_\name: # nocheckin: make error code available to function
    push $0 # dummy error code
    push $\index
    jmp intr_handler_body
.endm

.macro errc_handler index name
.global intr_handler_\name
intr_handler_\name:
    push $\index
    jmp intr_handler_body
.endm

intr_handler_body:
    pusha
    call intr_handler_common
    popa
    add $8, %sp # remove index and error code from stack
    iret # pops EIP, CS, and EFLAGS registers

#define ASM
#include "interrupt_table.h"
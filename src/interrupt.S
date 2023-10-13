.extern intr_handler_common

.macro no_errc_handler index name
.global intr_handler_\name
intr_handler_\name: # nocheckin: make error code available to function
    pusha
    push $\index
    call intr_handler_common
    add $4, %sp
    popa
    iret # pops EIP, CS, and EFLAGS registers
.endm

.macro errc_handler index name
.global intr_handler_\name
intr_handler_\name:
    pusha
    push $\index
    call intr_handler_common
    add $4, %sp
    popa
    add $4, %sp # remove error code from stack
    iret # pops EIP, CS, and EFLAGS registers
.endm

#define ASM
#include "interrupt_table.h"
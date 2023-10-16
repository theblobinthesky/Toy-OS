#ifdef ASM
#define ERRC_HANDLER(index, name) errc_handler index name
#define NO_ERRC_HANDLER(index, name) no_errc_handler index name
#else
#define ERRC_HANDLER(index, name)                       \
    void intr_handler_##name();                         \
    void kintr_handler_##name();                        \
    set_intr_table_entry(index, intr_handler_##name);   \
    set_kintr(index, kintr_handler_##name) // nocheckin

#define NO_ERRC_HANDLER(index, name) ERRC_HANDLER(index, name)
#endif
 
NO_ERRC_HANDLER(0, divide_error);       // Fault, DE (divide error)
NO_ERRC_HANDLER(1, debug_exception);    // Fault/Trap, DB (debug exception)
NO_ERRC_HANDLER(2, nmi);                // Interrupt, (Non-maskable external)
NO_ERRC_HANDLER(3, breakpoint);         // Trap, BP (breakpoint)
NO_ERRC_HANDLER(4, overflow);           // Trap, OF (overflow)
NO_ERRC_HANDLER(5, bound_range_exc);    // Fault, BR (BOUND range exceeded)
NO_ERRC_HANDLER(6, inv_op);             // Fault, UD (invalid opcode)
NO_ERRC_HANDLER(7, no_math_cop);        // Fault, NM (no math coprocessor)
ERRC_HANDLER(8, double_fault);          // Abort, DF (double fault)
NO_ERRC_HANDLER(9, cop_seg_overrun);    // Fault, (coprocessor segment overrun)
ERRC_HANDLER(10, inv_tss);              // Fault, TS (invalid tss)
ERRC_HANDLER(11, seg_not_present);      // Fault, NP (segment not present)
ERRC_HANDLER(12, stack_seg_fault);      // Fault, SS (stack-segment fault)
ERRC_HANDLER(13, general_protection);   // Fault, GP (general protection)
ERRC_HANDLER(14, page_fault);           // Fault, PF (page fault)
// 15 is intel reserved, do not use.
NO_ERRC_HANDLER(16, math_fault);        // Fault, MF (x87 extension floating-point error, math fault)
ERRC_HANDLER(17, alignment_check);      // Fault, AC (alignment check)
NO_ERRC_HANDLER(18, machine_check);     // Abort, MC (machine check)
NO_ERRC_HANDLER(19, simd_fp_exception); // Fault, XM (simd floating-point exception)
NO_ERRC_HANDLER(20, virt_exception);    // Fault, VE (virtualization exception)
// 21-31 are intel reserved, do not use.
// 32-47 are redirected
NO_ERRC_HANDLER(32, irq0);
NO_ERRC_HANDLER(33, keyboard);
NO_ERRC_HANDLER(34, irq2);
NO_ERRC_HANDLER(35, irq3);
NO_ERRC_HANDLER(36, irq4);
NO_ERRC_HANDLER(37, irq5);
NO_ERRC_HANDLER(38, irq6);
NO_ERRC_HANDLER(39, irq7);
NO_ERRC_HANDLER(40, irq8);
NO_ERRC_HANDLER(41, irq9);
NO_ERRC_HANDLER(42, irq10);
NO_ERRC_HANDLER(43, irq11);
NO_ERRC_HANDLER(44, irq12);
NO_ERRC_HANDLER(45, irq13);
NO_ERRC_HANDLER(46, irq14);
NO_ERRC_HANDLER(47, irq15);
// 48-255 are user defined.
NO_ERRC_HANDLER(128, syscall);
NO_ERRC_HANDLER(256, default);
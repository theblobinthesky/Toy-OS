#include "interrupt.h"
#include "utils.h"

#define KERNEL_MODE_DATA_SEGMENT 0x0
#define KERNEL_MODE_CODE_SEGMENT 0x8

typedef void(*interrupt_handler)();

#define INTERRUPT_TABLE_SIZE 256
interrupt_handler kintr_handlers[INTERRUPT_TABLE_SIZE];

struct intr_desc_32 {
   u16 offset_low;
   u16 selector;
   u8 zero;
   u8 type_attributes;
   u16 offset_high;
} __attribute__((packed));

struct intr_desc_32 intr_desc_table[INTERRUPT_TABLE_SIZE];

struct intr_desc_table_desc {
    u16 size;
    u32 offset;
} __attribute((packed));

void load_intr_desc_table(struct intr_desc_32* idt) {
    struct intr_desc_table_desc desc = {
        .size = sizeof(intr_desc_table) - 1,
        .offset = (u32)idt
    };

    __asm__ volatile("lidt (%0)" :: "r" (&desc) : "memory");
}

#define disable_interrupts() __asm__ volatile("cli")
#define enable_interrupts() __asm__ volatile("sti")

static void set_kintr(int index, interrupt_handler handler) {
    kintr_handlers[index] = handler;
}

struct intr_desc_32 get_intr_desc_32(void(*intr_handler)(), bool kernel_privilege, bool is_interrupt) {
    struct intr_desc_32 desc = {0};
    desc.offset_high = ((u32)intr_handler >> 16) & 0xffff;
    desc.selector = KERNEL_MODE_CODE_SEGMENT;
    desc.zero = 0;

    desc.type_attributes = 0x80; // set present bit
    // set user mode privilege, if necessary
    if(!kernel_privilege) desc.type_attributes |= (3 << 5); 
    // set gate type (always choose 32-bit gates)
    if(is_interrupt) desc.type_attributes |= 0xE;
    else desc.type_attributes |= 0xF; // trap gate

    desc.offset_low = (u32)intr_handler & 0xffff;

    return desc;
}

void set_intr_table_entry(int index, void(*intr_handler)()) {
    intr_desc_table[index] = get_intr_desc_32(intr_handler, true, true);
}

void intr_handler_common(int intr_index) {
    kintr_handlers[intr_index]();
}

static void kintr_handler_divide_error() {}
static void kintr_handler_debug_exception() {};
static void kintr_handler_nmi() {};
static void kintr_handler_breakpoint() {};
static void kintr_handler_overflow() {};
static void kintr_handler_bound_range_exc() {};
static void kintr_handler_inv_op() {};
static void kintr_handler_no_math_cop() {};
static void kintr_handler_double_fault() {};
static void kintr_handler_cop_seg_overrun() {};
static void kintr_handler_inv_tss() {};
static void kintr_handler_seg_not_present() {};
static void kintr_handler_stack_seg_fault() {};
static void kintr_handler_general_protection() {};
static void kintr_handler_page_fault() {};
static void kintr_handler_math_fault() {};
static void kintr_handler_alignment_check() {};
static void kintr_handler_machine_check() {};
static void kintr_handler_simd_fp_exception() {};
static void kintr_handler_virt_exception() {};

static void kintr_handler_irq0() {};        
static void kintr_handler_keyboard() {};        
static void kintr_handler_irq2() {};        
static void kintr_handler_irq3() {};        
static void kintr_handler_irq4() {};        
static void kintr_handler_irq5() {};        
static void kintr_handler_irq6() {};        
static void kintr_handler_irq7() {};        
static void kintr_handler_irq8() {};        
static void kintr_handler_irq9() {};        
static void kintr_handler_irq10() {};       
static void kintr_handler_irq11() {};       
static void kintr_handler_irq12() {};       
static void kintr_handler_irq13() {};       
static void kintr_handler_irq14() {};       
static void kintr_handler_irq15() {};

static void kintr_handler_syscall() {};
static void kintr_handler_default() {
    // empty
}

void interrupt_init() {
    // disable_interrupts(); TODO: this breaks stuff

    void intr_handler_default();
    void kintr_handler_default();
    
    for(int i = 0; i < INTERRUPT_TABLE_SIZE; i++) {
        set_intr_table_entry(i, intr_handler_default); // NOTE: this assumes all interrupts have no error code which might cause crashes
        set_kintr(i, kintr_handler_default);
    }

    // set the appropriate handlers
    #include "interrupt_table.h"

    load_intr_desc_table(intr_desc_table);

    // enable_interrupts(); TODO: this breaks stuff 
}
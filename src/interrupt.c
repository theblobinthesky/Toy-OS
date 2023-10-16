#include "interrupt.h"
#include "utils.h"
// configuring the programmable interrupt controller
#define PIC_OFFSET 32
#define INTR_INDEX_TO_IRQ(index) ((index) - PIC_OFFSET)
#define IRQ_TO_INTR_INDEX(irq) ((irq) + PIC_OFFSET)

#define PIC1_COMMAND 0x20 // master pic
#define PIC1_DATA (PIC1_COMMAND + 1)
#define PIC2_COMMAND 0xA0 // slave pic
#define PIC2_DATA (PIC2_COMMAND + 1)

#define ICW1_ICW4 0x01      		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02	    	/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		    /* Level triggered (edge) mode */
#define ICW1_INIT	0x10		    /* Initialization - required! */
 
#define ICW4_8086 0x01  		    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02	    	    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08	    	/* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C		/* Buffered mode/master */
#define ICW4_SFNM 0x10		        /* Special fully nested (not) */
 
void pic_remap(int offset) { 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset + 8);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
 
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();
 
	outb(PIC1_DATA, 0);
	outb(PIC2_DATA, 0);
}

void pic_send_end_of_interrupt(unsigned char irq) {
#define PIC_EOI 0x20

 	if(irq >= 8) outb(PIC2_COMMAND,PIC_EOI);
	outb(PIC1_COMMAND,PIC_EOI);
}

#define KERNEL_MODE_CODE_SEGMENT 8
#define KERNEL_MODE_DATA_SEGMENT 16

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

struct intr_desc_32 intr_desc_table[INTERRUPT_TABLE_SIZE] __attribute__((aligned(8)));

struct intr_desc_table_desc {
    u16 size;
    u32 offset;
}__attribute((packed));

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
    desc.offset_low = ((u32)intr_handler) & 0xffff;
    desc.selector = KERNEL_MODE_CODE_SEGMENT;
    desc.zero = 0;

    desc.type_attributes = 0x80; // set present bit
    // set user mode privilege, if necessary
    if(!kernel_privilege) desc.type_attributes |= (3 << 5); 
    // set gate type (always choose 32-bit gates)
    if(is_interrupt) desc.type_attributes |= 0xE;
    else desc.type_attributes |= 0xF; // trap gate

    desc.offset_high = ((u32)intr_handler >> 16) & 0xffff;

    return desc;
}

void set_intr_table_entry(int index, void(*intr_handler)()) {
    intr_desc_table[index] = get_intr_desc_32(intr_handler, true, true);
}

struct register_state {
    int edi;
	int esi;
	int ebp;
	int esp;
	int ebx;
	int edx;
	int ecx;
	int eax;
};

void intr_handler_common(struct register_state regs, int intr_index, int error_code) {
    if(intr_index < 0 || intr_index >= INTERRUPT_TABLE_SIZE) {
        // panic?
    }

    interrupt_handler handler = kintr_handlers[intr_index];
    if(!handler) {
        // panic?
    }

    handler();

    if(intr_index >= PIC_OFFSET && intr_index < PIC_OFFSET + 16) {
        pic_send_end_of_interrupt(INTR_INDEX_TO_IRQ(intr_index));
    } else {
        BOCHS_BREAK();
    }
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
    disable_interrupts();

    void intr_handler_default();
    
    for(int i = 0; i < INTERRUPT_TABLE_SIZE; i++) {
        set_intr_table_entry(i, intr_handler_default); // NOTE: this assumes all interrupts have no error code which might cause crashes
        set_kintr(i, 0);
    }

    // set the appropriate handlers
    #include "interrupt_table.h"

    load_intr_desc_table(intr_desc_table);

    pic_remap(PIC_OFFSET);

    enable_interrupts();
}
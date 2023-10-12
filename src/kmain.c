int wait_forever() {
    while(1) {
        __asm__("nop");
    }
}

char inp(int port) {
    char result;
    __asm__ volatile("inb %1, %0"
                 : "=a" (result)
                 : "Nd" (port));
    return result;
}

void out(int port, char value) {
    __asm__ volatile("out %1, %0"
                 :
                 : "Nd" (port), "a" (value));
}

#define FB_WIDTH  80
#define FB_HEIGHT 60
#define FB_STRIDE (FB_WIDTH / 8)
#define INPUT_STATUS_ONE_REG_PORT 0x3da

void clear_display() {
    char* vga_buffer = (char*) 0xa0000;

    for(int i = 0; i < 64 * 1024; i++) {
        vga_buffer[i] = 0xff;
    }

    // for(int y = 0; y < FB_HEIGHT * 16; y++) {
    //     for(int x = 0; x < FB_STRIDE; x++) {
    //         vga_text_buffer[y * FB_STRIDE + x] = 0xff;
    //     }
    // }
}

struct write_temporary {
    char data;
    char old_addr;
};

// works for sequencer, graphics and crt controller registers
struct vga_reg {
    int addr_port;
    int data_port;
    int reg_index;
};

struct write_temporary begin_vga_reg_write(struct vga_reg* reg) {
    // Input the value of the Address Register and save it for step 6
    char old_addr = inp(reg->addr_port);

    // Output the index of the desired Data Register to the Address Register.
    out(reg->addr_port, reg->reg_index);

    // Read the value of the Data Register and save it for later restoration upon termination, if needed.
    struct write_temporary ret = {
        .data = inp(reg->data_port),
        .old_addr = old_addr
    };

    return ret;
}

void end_vga_reg_write(struct vga_reg* reg, struct write_temporary* write) {
    // If writing, write the new value from step 4 to the Data register.
    out(reg->data_port, write->data);

    // Write the value of Address register saved in step 1 to the Address Register.
    out(reg->addr_port, write->old_addr);
}

void vga_reg_write(int addr_port, int data_port, int reg_index, char data) {
    struct vga_reg reg = { addr_port, data_port, reg_index };
    struct write_temporary temp = begin_vga_reg_write(&reg);
    temp.data = data;
    end_vga_reg_write(&reg, &temp);
}

struct attr_reg {
    int addr_data_reg_port;
    int data_reg_port;
    int reg_index;
};

struct write_temporary begin_attrib_write(struct attr_reg* reg) {
    // Input a value from the Input Status #1 Register (normally port 3DAh) and discard it.
    inp(INPUT_STATUS_ONE_REG_PORT);

    // Read the value of the Address/Data Register and save it for step 7.
    char old_addr_data = inp(reg->addr_data_reg_port);
    
    // Output the index of the desired Data Register to the Address/Data Register
    out(reg->addr_data_reg_port, reg->reg_index);
    
    // Read the value of the Data Register and save it for later restoration upon termination, if needed.
    struct write_temporary ret = {
        .data = inp(reg->data_reg_port),
        .old_addr = old_addr_data
    };

    return ret;
}

void end_attrib_write(struct attr_reg* reg, struct write_temporary* write) {
    // If writing, write the new value from step 5 to the Address/Data register.
    out(reg->addr_data_reg_port, write->data);
    
    // Write the value of Address register saved in step 1 to the Address/Data Register.
    out(reg->addr_data_reg_port, write->old_addr);
}

void attrib_reg_write(int addr_data_reg_port, int data_reg_port, int reg_index, char data) {
    struct attr_reg reg = { addr_data_reg_port, data_reg_port, reg_index }; 
    struct write_temporary temp = begin_attrib_write(&reg);
    temp.data = data;
    end_attrib_write(&reg, &temp);
}

void external_reg_write(int write_port, char data) {
    out(write_port, data);
}

int kmain() {
    char* vga_text_buffer = (char*) 0xb8000;
    *vga_text_buffer = 'K';

    // todo: disable screen
    // todo: unlock crtc registers
#define CRTC_ADDRESS_PORT 0x3d4
#define CRTC_DATA_PORT 0x3d5
    struct vga_reg vertical_retrace_end_reg = { CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x11 };
    struct write_temporary temp = begin_vga_reg_write(&vertical_retrace_end_reg);
    temp.data &= ~(1 << 7);
    end_vga_reg_write(&vertical_retrace_end_reg, &temp);

    // set the vga device to 640x480 planar 16 color mode
    // see: https://wiki.osdev.org/VGA_Hardware
    // attribute registers:
#define ATTRIBUTE_CTRL_ADDRESS_DATA_PORT 0x3c0
#define ATTRIBUTE_CTRL_DATA_READ_PORT 0x3c1
#define ATTRIBUTE_MODE_CONTROL_REG_INDEX 0x10
    attrib_reg_write(ATTRIBUTE_CTRL_ADDRESS_DATA_PORT, ATTRIBUTE_CTRL_DATA_READ_PORT, ATTRIBUTE_MODE_CONTROL_REG_INDEX, 0x01);

#define OVERSCAN_COLOR_REG_INDEX 0x11
    attrib_reg_write(ATTRIBUTE_CTRL_ADDRESS_DATA_PORT, ATTRIBUTE_CTRL_DATA_READ_PORT, OVERSCAN_COLOR_REG_INDEX, 0x00);

#define COLOR_PLANE_ENABLE_REG_INDEX 0x12
    attrib_reg_write(ATTRIBUTE_CTRL_ADDRESS_DATA_PORT, ATTRIBUTE_CTRL_DATA_READ_PORT, COLOR_PLANE_ENABLE_REG_INDEX, 0x0F);

#define HORIZONTAL_PANNING_REG_INDEX 0x13
    attrib_reg_write(ATTRIBUTE_CTRL_ADDRESS_DATA_PORT, ATTRIBUTE_CTRL_DATA_READ_PORT, HORIZONTAL_PANNING_REG_INDEX, 0x00);
    // todo: try that

#define COLOR_SELECT_REG_INDEX 0x14
    attrib_reg_write(ATTRIBUTE_CTRL_ADDRESS_DATA_PORT, ATTRIBUTE_CTRL_DATA_READ_PORT, COLOR_SELECT_REG_INDEX, 0x00);
    // todo: try that

    // external registers:
#define MISC_OUTPUT_REG_WRITE_PORT 0x3c2
    external_reg_write(MISC_OUTPUT_REG_WRITE_PORT, 0xe3);
    // todo: investigate

    // sequencer registers:
#define SEQUENCER_ADDRESS_PORT 0x3c4
#define SEQUENCER_DATA_PORT 0x3c5
#define CLOCKING_MODE_REG_INDEX 0x01
    vga_reg_write(SEQUENCER_ADDRESS_PORT, SEQUENCER_DATA_PORT, CLOCKING_MODE_REG_INDEX, 0x01);
    // todo: investigate

#define CHAR_MAP_SELECT_REG_INDEX 0x03
    vga_reg_write(SEQUENCER_ADDRESS_PORT, SEQUENCER_DATA_PORT, CHAR_MAP_SELECT_REG_INDEX, 0x00);
    // todo: investigate

#define MEMORY_MODE_REG_INDEX 0x04
    vga_reg_write(SEQUENCER_ADDRESS_PORT, SEQUENCER_DATA_PORT, MEMORY_MODE_REG_INDEX, 0x02);

    // graphics registers:
#define GRAPHICS_ADDRESS_PORT 0x3ce
#define GRAPHICS_DATA_PORT 0x3cf
#define GRAPHICS_MODE_REG_INDEX 0x05
    // read and write mode 0
    // disable 256 colors and shift interleave mode
    vga_reg_write(GRAPHICS_ADDRESS_PORT, GRAPHICS_DATA_PORT, GRAPHICS_MODE_REG_INDEX, 0x00);

#define MISC_GRAPHICS_REG_INDEX 0x06
    // select 0xa0000-0xaffff (64k large) region
    // disable alphanumeric mode
    vga_reg_write(GRAPHICS_ADDRESS_PORT, GRAPHICS_DATA_PORT, MISC_GRAPHICS_REG_INDEX, 0x05);

    // crt controller (crtc) registers:
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x00, 0x5f);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x01, 0x4f);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x02, 0x50);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x03, 0x82);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x04, 0x54);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x05, 0x80);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x06, 0x0b);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x07, 0x3e);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x08, 0x00);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x09, 0x40);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x10, 0xea);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x11, 0x8c);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x12, 0xdf);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x13, 0x28);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x14, 0x00);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x15, 0xe7);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x16, 0x04);
    vga_reg_write(CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x17, 0xe3);


    clear_display();    
    wait_forever();
}
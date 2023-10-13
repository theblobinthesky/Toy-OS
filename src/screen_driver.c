#include "screen_driver.h"

#define FB_BITS_PER_GROUP 8
#define FB_STRIDE (FB_WIDTH / FB_BITS_PER_GROUP)

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

static void end_vga_reg_write(struct vga_reg* reg, struct write_temporary* write) {
    // If writing, write the new value from step 4 to the Data register.
    out(reg->data_port, write->data);

    // Write the value of Address register saved in step 1 to the Address Register.
    out(reg->addr_port, write->old_addr);
}

static void vga_reg_write(int addr_port, int data_port, int reg_index, char data) {
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
#define INPUT_STATUS_ONE_REG_PORT 0x3da
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

static void end_attrib_write(struct attr_reg* reg, struct write_temporary* write) {
    // If writing, write the new value from step 5 to the Address/Data register.
    out(reg->addr_data_reg_port, write->data);
    
    // Write the value of Address register saved in step 1 to the Address/Data Register.
    out(reg->addr_data_reg_port, write->old_addr);
}

static void attrib_reg_write(int addr_data_reg_port, int data_reg_port, int reg_index, char data) {
    struct attr_reg reg = { addr_data_reg_port, data_reg_port, reg_index }; 
    struct write_temporary temp = begin_attrib_write(&reg);
    temp.data = data;
    end_attrib_write(&reg, &temp);
}

static void external_reg_write(int write_port, char data) {
    out(write_port, data);
}

static void enable_color_plane(int index) {
#define SEQUENCER_ADDRESS_PORT 0x3c4
#define SEQUENCER_DATA_PORT 0x3c5
#define MAP_MASK_REG_INDEX 0x02
    vga_reg_write(SEQUENCER_ADDRESS_PORT, SEQUENCER_DATA_PORT, MAP_MASK_REG_INDEX, 1 << index);
}

static void modify_crtc_registers(bool lock) {
#define CRTC_ADDRESS_PORT 0x3d4
#define CRTC_DATA_PORT 0x3d5
    struct vga_reg vertical_retrace_end_reg = { CRTC_ADDRESS_PORT, CRTC_DATA_PORT, 0x11 };
    struct write_temporary temp = begin_vga_reg_write(&vertical_retrace_end_reg);
    
    if(lock) temp.data &= ~(1 << 7);
    else temp.data |= (1 << 7);
    
    end_vga_reg_write(&vertical_retrace_end_reg, &temp);    
}

static void init_vga_device() {
    // todo: disable screen
    modify_crtc_registers(true);

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

    modify_crtc_registers(false);
}

// channels are r then g then b
#define PALETTE_SIZE 16
static int sixteen_colors_to_24bit_conversion[PALETTE_SIZE] = {
    0x000000, 0x800000, 0x008000, 0x808000,
    0x000080, 0x800080, 0x008080, 0xc0c0c0,
    0x808080, 0xff0000, 0x00ff00, 0xffff00,
    0x0000ff, 0xff00ff, 0x00ffff, 0xffffff
};

#define GET_CH_R(col) (((col) >> 16) & 0xff)
#define GET_CH_G(col) (((col) >> 8) & 0xff)
#define GET_CH_B(col) (((col) >> 0) & 0xff)

static void init_vga_palette() {
#define DAC_ADDR_WRITE_MODE_REG_PORT 0x3c8
#define DAC_DATA_REG_PORT 0x3c9

    for(char i = 0; i < PALETTE_SIZE; i++) {
        int color = sixteen_colors_to_24bit_conversion[i];
        out(DAC_ADDR_WRITE_MODE_REG_PORT, i);
        out(DAC_DATA_REG_PORT, GET_CH_R(color) >> 2); // color values are only 6-bit wide, so divide by 4
        out(DAC_DATA_REG_PORT, GET_CH_G(color) >> 2);
        out(DAC_DATA_REG_PORT, GET_CH_B(color) >> 2);
    }
}

// According to http://www.osdever.net/FreeVGA/vga/seqreg.htm#04 this can be useful for rapid full-screen updates.
static void modify_screen_disable(bool disable) {
    struct vga_reg clocking_mode_reg = { SEQUENCER_ADDRESS_PORT, SEQUENCER_DATA_PORT, 0x01 };
    struct write_temporary temp = begin_vga_reg_write(&clocking_mode_reg);
    
    if(disable) temp.data |= (1 << 5);
    else temp.data &= ~(1 << 5);

    end_vga_reg_write(&clocking_mode_reg, &temp);
}

static char lookup_color(char r, char g, char b) {
    int min_dist_sqred = 999999;
    char index = -1;

    for(char i = 0; i < 16; i++) {
        int p_c = sixteen_colors_to_24bit_conversion[i];
        int p_r = GET_CH_R(p_c);
        int p_g = GET_CH_G(p_c);
        int p_b = GET_CH_B(p_c);

        int d_r = r - p_r;
        int d_g = g - p_g;
        int d_b = b - p_b;
        int dist_sqred = d_r * d_r + d_g * d_g + d_b * d_b;
    
        if(dist_sqred <= min_dist_sqred) {
            min_dist_sqred = dist_sqred;
            index = i;
        }
    }

    return index;
}

char* screen_framebuffer = (char*)0x100000;

static void clear_screen() {
    for(int y = 0; y < FB_HEIGHT; y++) {
        for(int x = 0; x < FB_WIDTH; x++) {
            screen_get_fb(x, y, FB_CH_R) = 0;
            screen_get_fb(x, y, FB_CH_G) = 0; 
            screen_get_fb(x, y, FB_CH_B) = 0;
        }
    }
}

static void write_to_vga_buffer(int vga_plane, struct rect r) {
    volatile char* vga_buffer = (char*) 0xa0000;

    for(int i = 0; i < FB_HEIGHT * FB_STRIDE; i++) vga_buffer[i] = 0;

    for(int y = r.y; y < rect_get_bottom(r); y++) {
        for(int s = r.x / FB_BITS_PER_GROUP; s < (rect_get_right(r) + FB_BITS_PER_GROUP) / FB_BITS_PER_GROUP; s++) {
            int i = y * FB_STRIDE + s;
            
            char byte = 0;

            for(int b = 0; b < FB_BITS_PER_GROUP; b++) {
                int x = s * FB_BITS_PER_GROUP + b;
                
                char fb_r = screen_get_fb(x, y, FB_CH_R);
                char fb_g = screen_get_fb(x, y, FB_CH_G);
                char fb_b = screen_get_fb(x, y, FB_CH_B);
                char palette_ind = lookup_color(fb_r, fb_g, fb_b);
                palette_ind = (palette_ind >> vga_plane) & 1;

                byte |= (palette_ind << (7 - b));
            }

            vga_buffer[i] = byte;
        }
    }
}

char* screen_get_screen_framebuffer() {
    return screen_framebuffer;
}

void screen_bit_blit() {
    struct rect screen_rect = { 0, 0, FB_WIDTH, FB_HEIGHT };
    screen_bit_blit_rect(screen_rect);
}

void screen_bit_blit_rect(struct rect r) {
    enable_color_plane(0);
    write_to_vga_buffer(0, r);

    enable_color_plane(1);
    write_to_vga_buffer(1, r);

    enable_color_plane(2);
    write_to_vga_buffer(2, r);

    enable_color_plane(3);
    write_to_vga_buffer(3, r);
}

void screen_init_driver() {
    modify_screen_disable(true);
    init_vga_device();
    init_vga_palette();
    clear_screen();
    screen_bit_blit();    
    modify_screen_disable(false);
}
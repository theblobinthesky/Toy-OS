#include "screen_driver.h"

int wait_forever() {
    while(1) {
        __asm__ volatile("nop");
    }
}

void show_color_pattern() {
    for(int y = 0; y < FB_HEIGHT; y++) {
        for(int x = 0; x < FB_WIDTH; x++) {
            screen_get_fb(x, y, FB_CH_R) = x % 256;
            screen_get_fb(x, y, FB_CH_G) = y % 256;
            screen_get_fb(x, y, FB_CH_B) = (x + y) % 256;
        }
    }

    screen_bit_blit();
}

void kmain() {
    screen_init_driver();
    show_color_pattern();

    wait_forever();
}
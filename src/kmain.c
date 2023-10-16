#include "interrupt.h"
#include "screen_driver.h"

int wait_forever() {
    while(1) {
        __asm__ volatile("nop");
    }
}

void show_color_pattern(struct rect r, bool mode) {
    for(int y = r.y; y < rect_get_bottom(r); y++) {
        for(int x = r.x; x < rect_get_right(r); x++) {
            if(mode) {
                screen_get_fb(x, y, FB_CH_R) = x % 256;
                screen_get_fb(x, y, FB_CH_G) = y % 256;
                screen_get_fb(x, y, FB_CH_B) = (x + y) % 256;
            } else {
                screen_get_fb(x, y, FB_CH_G) = x % 256;
                screen_get_fb(x, y, FB_CH_B) = y % 256;
                screen_get_fb(x, y, FB_CH_R) = (x + y) % 256;
            }
        }
    }

    screen_bit_blit_rect(r);
}

void kmain() {
    interrupt_init();

    // wait until we have paged memory mapping to enable this again!
    // screen_init_driver();

    // const int padding = 60;
    // struct rect r2 = { padding, padding, FB_WIDTH - padding * 2, FB_HEIGHT - padding * 2 };
    // show_color_pattern(r2, false);

    // struct rect r = SCREEN_FB_RECT;
    // show_color_pattern(r, true);

    wait_forever();
}
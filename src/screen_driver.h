#pragma once

#define FB_WIDTH 640
#define FB_HEIGHT 480
#define FB_CHANNELS 3
#define FB_CH_R 0
#define FB_CH_G 1
#define FB_CH_B 2
extern char* screen_framebuffer;
#define screen_get_fb(x, y, c) screen_framebuffer[y * FB_WIDTH * FB_CHANNELS + x * FB_CHANNELS + c]

char* screen_get_screen_framebuffer();
void screen_bit_blit();
void screen_init_driver();
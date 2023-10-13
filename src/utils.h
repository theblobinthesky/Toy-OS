#pragma once
#define BOCHS_BREAK() __asm__ volatile("xchgw %bx, %bx")

#define bool int
#define true 1
#define false 0
typedef __INT8_TYPE__ s8;
typedef __UINT8_TYPE__ u8;
typedef __INT16_TYPE__ s16;
typedef __UINT16_TYPE__ u16;
typedef __INT32_TYPE__ s32;
typedef __UINT32_TYPE__ u32;

inline static char inp(int port) {
    char result;
    __asm__ volatile("inb %1, %0"
                 : "=a" (result)
                 : "Nd" (port));
    return result;
}

inline static void out(int port, char value) {
    __asm__ volatile("out %1, %0"
                 :
                 : "Nd" (port), "a" (value));
}

struct rect {
    int x, y;
    int w, h;
};

#define rect_get_right(r) (r.x + r.w)
#define rect_get_bottom(r) (r.y + r.h)
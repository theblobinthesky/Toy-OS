#pragma once

#define bool int
#define true 1
#define false 0

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

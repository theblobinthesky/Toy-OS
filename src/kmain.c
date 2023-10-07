const char* test = "Hello world!";

int testf(char* vga_text_buffer) {
    while(1) {
        *vga_text_buffer = 'K';
    }
}

int kmain() {
    char* vga_text_buffer = (char*) 0xb8000;
    testf(vga_text_buffer);
}
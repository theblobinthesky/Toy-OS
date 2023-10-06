const char* test = "Hello world!";

int main() {
    char* vga_text_buffer = (char*) 0xb8000;
    
    while(1) {
        *vga_text_buffer = 'K';
    }
}
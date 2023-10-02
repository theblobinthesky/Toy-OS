#include <stdio.h>

#define SECTOR_SIZE 512

char zero = 0;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Wrong number of arguments for padder!\n");
        return 1;
    }

    char* path = argv[1];
    FILE* file = fopen(path, "a+");
    if(!file) {
        fprintf(stderr, "File path %s does not exist!", path);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);

    int pad = SECTOR_SIZE - (size % SECTOR_SIZE);
    if (pad == SECTOR_SIZE) pad = 0;

    if (pad > 0) {
        fwrite(&zero, 1, pad, file);
        fclose(file);
    }

    printf("Padded %s with old size %d and sector size %d to %d.\n", path, size, SECTOR_SIZE, size + pad);

    return 0;
}
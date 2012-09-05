#define FALSE          0
#define TRUE           1
#define ALPHA_VALUE    128

#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    FILE *lut = NULL;
    uint8_t lutBufferRGBA[1024];
    int32_t i = 0;

    for(i = 0; i < 256; i++)
        lutBufferRGBA[i] = 0x00;
    for(i = 256; i < 512; i++)
        lutBufferRGBA[i] = 0xFF;
    for(i = 512; i < 768; i++)
        lutBufferRGBA[i] = 0x00;
    for(i = 768; i < 1024; i++)
        lutBufferRGBA[i] = 0x80;

    lut = fopen(argv[1], "w+b");
    if(lut == NULL) {
        printf("Unable to open file at %s\n", argv[1]);
        return FALSE;
    }
    if(fwrite(lutBufferRGBA, 1, 1024, lut) != 1024) {
        printf("Unable to write RGBA file.\n");
        if(fclose(lut) != 0)
            printf("Additionally, an error occured closing the file.\n");
        return FALSE;
    }
    if(fclose(lut) != 0)
        printf("Error closing the RGBA file\n");

    return TRUE;
}

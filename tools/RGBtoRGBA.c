#define FALSE          0
#define TRUE           1
#define ALPHA_VALUE    128

#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    FILE *lut = NULL;
    char RGBAfile[1024];
    uint8_t lutBufferRGB[768];
    uint8_t lutBufferRGBA[1024];
    int32_t readBytes = 0, i = 0;

    printf("Reading LUT at %s\n", argv[1]);

    lut = fopen(argv[1], "rb");
    if(lut == NULL) {
        printf("Unable to open LUT at %s\n", argv[1]);
        return FALSE;
    }

    readBytes = (int32_t)fread(lutBufferRGB, 1, 768, lut);
    if(readBytes != 768) {
        printf("File %s does not have 768 bytes.\n", argv[1]);
        if(fclose(lut) != 0) {
            printf("Additionally, an error occured closing the file");
        }
        return FALSE;
    }

    memcpy(lutBufferRGBA, lutBufferRGB, 768);
    for(i = 768; i < 1024; i++)
        lutBufferRGBA[i] = 0x80;

    if(fclose(lut) != 0) {
        printf("Error closing LUT file.\n");
        return FALSE;
    }

    sprintf(RGBAfile, "%s.rgba", argv[1]);

    lut = fopen(RGBAfile, "w+b");
    if(lut == NULL) {
        printf("Unable to open RGBA file at %s\n", RGBAfile);
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

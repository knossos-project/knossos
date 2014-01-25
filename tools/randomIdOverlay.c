#define FALSE          0
#define TRUE           1
#define OBJID_BYTES    3

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int32_t makeRandomSubCube(uint8_t *cube, int32_t cubeEdge, int32_t subCubeEdgeLen, int32_t x, int32_t y, int32_t z);

int main(int argc, char **argv) {
    FILE *cube = NULL;
    uint8_t cubeBuffer[128 * 128 * 128 * OBJID_BYTES];
    int32_t i = 0, x, y, z;
    time_t seconds;

    memset(cubeBuffer, '\0', 128 * 128 * 128 * OBJID_BYTES);

    for(i = 0; i < 200; i++) {
        time(&seconds);
        srand((unsigned int)seconds + i);
        x = rand() % 129;
        y = rand() % 129;
        z = rand() % 129;

        printf("Generating sub cube %d at (%d, %d, %d)\n", i, x, y, z);
        makeRandomSubCube(cubeBuffer, 128, 32, x, y, z);
    }

    cube = fopen(argv[1], "w+b");
    if(cube == NULL) {
        printf("Unable to open overlay cube file at %s\n", argv[1]);
        return FALSE;
    }
    if(fwrite(cubeBuffer, 1, 128 * 128 * 128 * OBJID_BYTES, cube) !=
              128 * 128 * 128 * OBJID_BYTES) {
        printf("Unable to write cube file.\n");
        if(fclose(cube) != 0)
            printf("Additionally, an error occured closing the file.\n");
        return FALSE;
    }
    if(fclose(cube) != 0)
        printf("Error closing the RGBA file\n");

    return TRUE;
}


int32_t makeRandomSubCube(uint8_t *cube,
                          int32_t cubeEdge,
                          int32_t len,
                          int32_t x,
                          int32_t y,
                          int32_t z) {

    int8_t id[3];
    int32_t i, j, k;
    int32_t cubeArea;
    time_t seconds;

    time(&seconds);
    srand((unsigned int)seconds + x);

    cubeArea = cubeEdge * cubeEdge;

    /* Pick random id */
    id[0] = 1 + (rand() % 255);
    id[1] = 1 + (rand() % 255);
    id[2] = 1 + (rand() % 255);

    /* Fill subcube with random id */

    for(i = x; i < x + len; i++) {
        for(j = y; j < y + len; j++) {
            for(k = z; k < z + len; k++) {
                if(i >= cubeEdge ||
                   j >= cubeEdge ||
                   k >= cubeEdge)
                    continue;

                //printf("(%d, %d, %d)\n", i, j, k);                
                memcpy(&cube[(i + j * cubeEdge + k * cubeArea) * OBJID_BYTES], id, 3); 
            }
        }
    }

    return TRUE;
}

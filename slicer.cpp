#include "slicer.h"

extern struct stateInfo *state;

Slicer::Slicer(QObject *parent) :
    QObject(parent)
{
}

bool Slicer::sliceExtract_standard(Byte *datacube,
                                       Byte *slice,
                                       vpConfig *vpConfig) {
    int i, j;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1]
                     = slice[2]
                     = *datacube;

            datacube++;
            slice += 3;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                slice[0] = slice[1]
                         = slice[2]
                         = *datacube;

                datacube++;
                slice += 3;
            }
            datacube = datacube
                       + state->cubeSliceArea
                       - state->cubeEdgeLength;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = slice[1]
                     = slice[2]
                     = *datacube;
            datacube += state->cubeEdgeLength;
            slice += 3;
        }
        break;
    }
    return true;
}


bool Slicer::sliceExtract_adjust(Byte *datacube,
                                     Byte *slice,
                                     vpConfig *vpConfig) {
    int i, j;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
            slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
            slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

            datacube++;
            slice += 3;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
                slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
                slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

                datacube++;
                slice += 3;
            }

            datacube  = datacube
                        + state->cubeSliceArea
                        - state->cubeEdgeLength;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            slice[0] = state->viewerState->datasetAdjustmentTable[0][*datacube];
            slice[1] = state->viewerState->datasetAdjustmentTable[1][*datacube];
            slice[2] = state->viewerState->datasetAdjustmentTable[2][*datacube];

            datacube += state->cubeEdgeLength;
            slice += 3;
        }
        break;
    }
    return true;
}


bool Slicer::dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    if(state->viewerState->datasetAdjustmentOn) {
        /* Texture type GL_RGB and we need to adjust coloring */
        sliceExtract_adjust(datacube, slice, vpConfig);
    }
    else {
        /* Texture type GL_RGB and we don't need to adjust anything*/
        sliceExtract_standard(datacube, slice, vpConfig);
    }
    return true;
}

bool Slicer::ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    int i, j;
    int objId, *objIdP;

    objIdP = &objId;
    datacube += dcOffset;

    switch(vpConfig->type) {
    case SLICE_XY:
        for(i = 0; i < state->cubeSliceArea; i++) {
            memcpy(objIdP, datacube, OBJID_BYTES);
            slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
            slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
            slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
            slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

            //printf("(%d, %d, %d, %d)", slice[0], slice[1], slice[2], slice[3]);

            datacube += OBJID_BYTES;
            slice += 4;
        }
        break;
    case SLICE_XZ:
        for(j = 0; j < state->cubeEdgeLength; j++) {
            for(i = 0; i < state->cubeEdgeLength; i++) {
                memcpy(objIdP, datacube, OBJID_BYTES);
                slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
                slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
                slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
                slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

                datacube += OBJID_BYTES;
                slice += 4;
            }

            datacube = datacube
                       + state->cubeSliceArea * OBJID_BYTES
                       - state->cubeEdgeLength * OBJID_BYTES;
        }
        break;
    case SLICE_YZ:
        for(i = 0; i < state->cubeSliceArea; i++) {
            memcpy(objIdP, datacube, OBJID_BYTES);
            slice[0] = state->viewerState->overlayColorMap[0][objId % 256];
            slice[1] = state->viewerState->overlayColorMap[1][objId % 256];
            slice[2] = state->viewerState->overlayColorMap[2][objId % 256];
            slice[3] = state->viewerState->overlayColorMap[3][objId % 256];

            datacube += state->cubeEdgeLength * OBJID_BYTES;
            slice += 4;
        }

        break;
    }
    return true;
}

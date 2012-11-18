#include "viewer.h"
#include "knossos.h"

extern  stateInfo *tempConfig;
extern  stateInfo *state;

Viewer::Viewer(QObject *parent) :
    QThread(parent)
{
    viewer();
}

static vpList* vpListNew() {
     vpList *newVpList = NULL;

     newVpList = (vpList *) malloc(sizeof( vpList));
     if(newVpList == NULL) {
         qDebug("Out of memory.\n");
         return NULL;
     }
     newVpList->entry = NULL;
     newVpList->elements = 0;

     return newVpList;
 }

static int32_t vpListAddElement(vpList *vpList, viewPort *viewPort, vpBacklog *backlog) {
     vpListElement *newElement;

     newElement = (vpListElement *) malloc(sizeof( vpListElement));
     if(newElement == NULL) {
         qDebug("Out of memory\n");
         // Do not return FALSE here. That's a bug. FAIL is hackish... Is there a
         // better way? */
         return FAIL;
     }

     newElement->viewPort = viewPort;
     newElement->backlog = backlog;

     if(vpList->entry != NULL) {
         vpList->entry->next->previous = newElement;
         newElement->next = vpList->entry->next;
         vpList->entry->next = newElement;
         newElement->previous = vpList->entry;
     } else {
         vpList->entry = newElement;
         vpList->entry->next = newElement;
         vpList->entry->previous = newElement;
     }

     vpList->elements = vpList->elements + 1;

     return vpList->elements;
 }

static vpBacklog *backlogNew() {
     vpBacklog *newBacklog;

     newBacklog = (vpBacklog *) malloc(sizeof( vpBacklog));
     if(newBacklog == NULL) {
         qDebug("Out of memory.\n");
         return NULL;
     }
     newBacklog->entry = NULL;
     newBacklog->elements = 0;

     return newBacklog;
 }

static vpList *vpListGenerate(viewerState *viewerState) {
     vpList *newVpList = NULL;
     vpBacklog *currentBacklog = NULL;
     int i = 0;

     newVpList = vpListNew();
     if(newVpList == NULL) {
         qDebug("Error generating new vpList.");
         _Exit(FALSE);
     }

     for(i = 0; i < viewerState->numberViewPorts; i++) {
         if(viewerState->viewPorts[i].type == VIEWPORT_SKELETON)
             continue;
         currentBacklog = backlogNew();
         if(currentBacklog == NULL) {
             qDebug("Error creating backlog.");
             _Exit(FALSE);
         }
         vpListAddElement(newVpList, &(viewerState->viewPorts[i]), currentBacklog);
     }

     return newVpList;
 }

static int32_t backlogDelElement(vpBacklog *backlog, vpBacklogElement *element) {
     if(element->next == element) {
         // This is the only element in the list
         backlog->entry = NULL;
     } else {
         element->next->previous = element->previous;
         element->previous->next = element->next;
         backlog->entry = element->next;
     }

     free(element);

     backlog->elements = backlog->elements - 1;

     return backlog->elements;
}

static bool backlogDel(vpBacklog *backlog) {
     while(backlog->elements > 0) {
         if(backlogDelElement(backlog, backlog->entry) < 0) {
             qDebug("Error deleting element at %p from the backlog. %d elements remain in the list.",
                 backlog->entry, backlog->elements);
             return false;
         }
     }

     free(backlog);

     return true;

}


static int32_t vpListDelElement( vpList *list,  vpListElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        list->entry = NULL;
    } else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        list->entry = element->next;
    }

    if(backlogDel(element->backlog) == FALSE) {
        qDebug("Error deleting backlog at %p of vpList element at %p.",
               element->backlog, element);
        return FAIL;
    }
    free(element);

    list->elements = list->elements - 1;

    return list->elements;
}



static bool vpListDel(vpList *list) {
     while(list->elements > 0) {
         if(vpListDelElement(list, list->entry) < 0) {
             qDebug("Error deleting element at %p from the slot list %d elements remain in the list.",
                    list->entry, list->elements);
             return false;
         }
     }

     free(list);

     return true;
 }

static int32_t backlogAddElement(vpBacklog *backlog, Coordinate datacube, uint32_t dcOffset, Byte *slice, uint32_t x_px, uint32_t y_px, uint32_t cubeType) {
     vpBacklogElement *newElement;

     newElement = (vpBacklogElement *) malloc(sizeof( vpBacklogElement));
     if(newElement == NULL) {
         qDebug("Out of memory.");
         /* Do not return FALSE here. That's a bug. FAIL is hackish... Is there a better way? */
         return FAIL;
     }

     newElement->slice = slice;
     SET_COORDINATE(newElement->cube, datacube.x, datacube.y, datacube.z);
     newElement->x_px = x_px;
     newElement->y_px = y_px;
     newElement->dcOffset = dcOffset;
     newElement->cubeType = cubeType;

     if(backlog->entry != NULL) {
         backlog->entry->next->previous = newElement;
         newElement->next = backlog->entry->next;
         backlog->entry->next = newElement;
         newElement->previous = backlog->entry;
     } else {
         backlog->entry = newElement;
         backlog->entry->next = newElement;
         backlog->entry->previous = newElement;
     }

     backlog->elements = backlog->elements + 1;

     return backlog->elements;
 }

static bool sliceExtract_standard(Byte *datacube,
                                       Byte *slice,
                                       viewPort *viewPort) {

     int32_t i, j;

     switch(viewPort->type) {
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


static bool sliceExtract_adjust(Byte *datacube,
                                     Byte *slice,
                                     viewPort *viewPort) {
     int32_t i, j;

     switch(viewPort->type) {
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


static bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, viewPort *viewPort) {
     datacube += dcOffset;

     if(state->viewerState->datasetAdjustmentOn) {
         /* Texture type GL_RGB and we need to adjust coloring */
         sliceExtract_adjust(datacube, slice, viewPort);
     }
     else {
         /* Texture type GL_RGB and we don't need to adjust anything*/
         sliceExtract_standard(datacube, slice, viewPort);
     }

     return true;
 }

static bool ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, viewPort *viewPort) {

    int32_t i, j;
    int32_t objId,
            *objIdP;

    objIdP = &objId;

    datacube += dcOffset;

    switch(viewPort->type) {
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

static bool vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState) {

     vpBacklogElement *currentElement = NULL,
                             *nextElement = NULL;
     Byte *cube = NULL;
     uint32_t elements = 0,
              i = 0;

     if(currentVp->backlog->entry == NULL) {
         qDebug("Called vpHandleBacklog, but there is no backlog.");
         return false;
     }

     elements = currentVp->backlog->elements;
     currentElement = currentVp->backlog->entry;

     for(i = 0; i < elements; i++)  {
         nextElement = currentElement->next;

         if(currentElement->cubeType == CUBE_DATA) {
             //SDL_LockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->lock();
             cube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentElement->cube);
             //SDL_UnlockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->unlock();


             if(cube == HT_FAILURE) {


                                    // if(currentElement->cube.x >= 3) {
                         //LOG("handleBL: currentDc %d, %d, %d", currentElement->cube.x, currentElement->cube.y, currentElement->cube.z);
                           //          }
                 //LOG("failed to get cube in viewer");
             }
             else {
                 dcSliceExtract(cube,
                                currentElement->slice,
                                currentElement->dcOffset,
                                currentVp->viewPort);

                 glBindTexture(GL_TEXTURE_2D, currentVp->viewPort->texture.texHandle);
                 glTexSubImage2D(GL_TEXTURE_2D,
                                 0,
                                 currentElement->x_px,
                                 currentElement->y_px,
                                 state->cubeEdgeLength,
                                 state->cubeEdgeLength,
                                 GL_RGB,
                                 GL_UNSIGNED_BYTE,
                                 currentElement->slice);
                 glBindTexture(GL_TEXTURE_2D, 0);
                 backlogDelElement(currentVp->backlog, currentElement);
             }
         }
         else if(currentElement->cubeType == CUBE_OVERLAY) {
             //SDL_LockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->lock();
             cube = Hashtable::ht_get((Hashtable*) state->Oc2Pointer, currentElement->cube);
             //SDL_UnlockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->unlock();

             if(cube == HT_FAILURE) {

             }
             else {
                 ocSliceExtract(cube,
                                currentElement->slice,
                                currentElement->dcOffset,
                                currentVp->viewPort);

                 glBindTexture(GL_TEXTURE_2D, currentVp->viewPort->texture.overlayHandle);
                 glTexSubImage2D(GL_TEXTURE_2D,
                                 0,
                                 currentElement->x_px,
                                 currentElement->y_px,
                                 state->cubeEdgeLength,
                                 state->cubeEdgeLength,
                                 GL_RGBA,
                                 GL_UNSIGNED_BYTE,
                                 currentElement->slice);
                 glBindTexture(GL_TEXTURE_2D, 0);
                 backlogDelElement(currentVp->backlog, currentElement);
             }
         }

         currentElement = nextElement;
     }

     if(currentVp->backlog->elements != 0)
         return false;
     else
         return true;


 }

static int32_t texIndex(uint32_t x,
                        uint32_t y,
                        uint32_t colorMultiplicationFactor,
                        viewPortTexture *texture) {
    uint32_t index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;

}

static bool vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState) {

     // Load the texture for a viewport by going through all relevant datacubes and copying slices
     // from those cubes into the texture.

     int32_t x_px = 0, x_dc = 0, y_px = 0, y_dc = 0;
     Coordinate upperLeftDc, currentDc, currentPosition_dc;
     Coordinate currPosTrans, leftUpperPxInAbsPxTrans;

     Byte *datacube = NULL, *overlayCube = NULL;
     int32_t dcOffset = 0, index = 0;


     CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
     DIV_COORDINATE(currPosTrans, state->magnification);

     CPY_COORDINATE(leftUpperPxInAbsPxTrans, currentVp->viewPort->texture.leftUpperPxInAbsPx);
     DIV_COORDINATE(leftUpperPxInAbsPxTrans, state->magnification);

     currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

     upperLeftDc = Coordinate::Px2DcCoord(leftUpperPxInAbsPxTrans);



     // We calculate the coordinate of the DC that holds the slice that makes up the upper left
     // corner of our texture.
     // dcOffset is the offset by which we can index into a datacube to extract the first byte of
     // slice relevant to the texture for this viewport.
     //
     // Rounding should be explicit!
     switch(currentVp->viewPort->type) {
         case SLICE_XY:
             dcOffset = state->cubeSliceArea
                        //* (viewerState->currentPosition.z - state->cubeEdgeLength
                        * (currPosTrans.z - state->cubeEdgeLength
                        * currentPosition_dc.z);
             break;

         case SLICE_XZ:
             dcOffset = state->cubeEdgeLength
                        * (currPosTrans.y  - state->cubeEdgeLength
                        //     * (viewerState->currentPosition.y  - state->cubeEdgeLength
                        * currentPosition_dc.y);
             break;

         case SLICE_YZ:
             dcOffset = //viewerState->currentPosition.x - state->cubeEdgeLength
                     currPosTrans.x - state->cubeEdgeLength
                        * currentPosition_dc.x;
             break;

         default:
             qDebug("No such slice view: %d.", currentVp->viewPort->type);
             return false;
     }

     // We iterate over the texture with x and y being in a temporary coordinate
     // system local to this texture.
     for(x_dc = 0; x_dc < currentVp->viewPort->texture.usedTexLengthDc; x_dc++) {
         for(y_dc = 0; y_dc < currentVp->viewPort->texture.usedTexLengthDc; y_dc++) {
             x_px = x_dc * state->cubeEdgeLength;
             y_px = y_dc * state->cubeEdgeLength;

             switch(currentVp->viewPort->type) {
                 // With an x/y-coordinate system in a viewport, we get the following
                 // mapping from viewport (slice) coordinates to global (dc)
                 // coordinates:
                 // XY-slice: x local is x global, y local is y global
                 // XZ-slice: x local is x global, y local is z global
                 // YZ-slice: x local is y global, y local is z global.
             case SLICE_XY:
                 SET_COORDINATE(currentDc,
                                upperLeftDc.x + x_dc,
                                upperLeftDc.y + y_dc,
                                upperLeftDc.z);
                 break;
             case SLICE_XZ:
                 SET_COORDINATE(currentDc,
                                upperLeftDc.x + x_dc,
                                upperLeftDc.y,
                                upperLeftDc.z + y_dc);
                 break;
             case SLICE_YZ:
                 SET_COORDINATE(currentDc,
                                upperLeftDc.x,
                                upperLeftDc.y + x_dc,
                                upperLeftDc.z + y_dc);
                 break;
             default:
                 LOG("No such slice type (%d) in vpGenerateTexture.", currentVp->viewPort->type);
             }

             //SDL_LockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->lock();
             datacube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
             overlayCube = Hashtable::ht_get(state->Oc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
             //SDL_UnlockMutex(state->protectCube2Pointer);
             state->protectCube2Pointer->unlock();

             /*
              *  Take care of the data textures.
              *
              */
             glBindTexture(GL_TEXTURE_2D,
                           currentVp->viewPort->texture.texHandle);

             glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

             // This is used to index into the texture. texData[index] is the first
             // byte of the datacube slice at position (x_dc, y_dc) in the texture.
             index = texIndex(x_dc, y_dc, 3, &(currentVp->viewPort->texture));

             if(datacube == HT_FAILURE) {

                 backlogAddElement(currentVp->backlog,
                                   currentDc,
                                   dcOffset,
                                   &(viewerState->texData[index]),
                                   x_px,
                                   y_px,
                                   CUBE_DATA);

                 glTexSubImage2D(GL_TEXTURE_2D,
                                 0,
                                 x_px,
                                 y_px,
                                 state->cubeEdgeLength,
                                 state->cubeEdgeLength,
                                 GL_RGB,
                                 GL_UNSIGNED_BYTE,
                                 viewerState->defaultTexData);
             }
             else {
                 dcSliceExtract(datacube,
                                &(viewerState->texData[index]),
                                dcOffset,
                                currentVp->viewPort);

                 glTexSubImage2D(GL_TEXTURE_2D,
                                 0,
                                 x_px,
                                 y_px,
                                 state->cubeEdgeLength,
                                 state->cubeEdgeLength,
                                 GL_RGB,
                                 GL_UNSIGNED_BYTE,
                                 &(viewerState->texData[index]));
             }

             /*
              *  Take care of the overlay textures.
              *
              */
             if(state->overlay) {
                 glBindTexture(GL_TEXTURE_2D,
                               currentVp->viewPort->texture.overlayHandle);

                 glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                 // This is used to index into the texture. texData[index] is the first
                 // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                 index = texIndex(x_dc, y_dc, 4, &(currentVp->viewPort->texture));

                 if(overlayCube == HT_FAILURE) {
                     backlogAddElement(currentVp->backlog,
                                       currentDc,
                                       dcOffset * OBJID_BYTES,
                                       &(viewerState->overlayData[index]),
                                       x_px,
                                       y_px,
                                       CUBE_OVERLAY);

                     glTexSubImage2D(GL_TEXTURE_2D,
                                     0,
                                     x_px,
                                     y_px,
                                     state->cubeEdgeLength,
                                     state->cubeEdgeLength,
                                     GL_RGBA,
                                     GL_UNSIGNED_BYTE,
                                     viewerState->defaultOverlayData);
                 }
                 else {
                     ocSliceExtract(overlayCube,
                                    &(viewerState->overlayData[index]),
                                    dcOffset * OBJID_BYTES,
                                    currentVp->viewPort);

                     glTexSubImage2D(GL_TEXTURE_2D,
                                     0,
                                     x_px,
                                     y_px,
                                     state->cubeEdgeLength,
                                     state->cubeEdgeLength,
                                     GL_RGBA,
                                     GL_UNSIGNED_BYTE,
                                     &(viewerState->overlayData[index]));
                 }
             }
         }
     }
     glBindTexture(GL_TEXTURE_2D, 0);
     return true;
 }

 /* For downsample & upsamleVPTexture:
  * we read the texture to a CPU side - buffer,
  * and send it to graphicscard after the resampling. Using
  * OpenGL is certainly possible for the resampling
  * but the CPU implementation appears to be
  * more straightforward, with probably almost no
  * performance penalty. We use a simple
  * box filter for the downsampling */

static bool downsampleVPTexture(viewPort *viewPort) {
    /* allocate 2 texture buffers */
    //Byte *orig, *resampled;

    /* get the texture */

    /* downsample it */

    /* send it to the graphicscard again */

    return true;
}


static bool upsampleVPTexture(viewPort *viewPort) {
   return true;
}

/* this function calculates the mapping between the left upper texture pixel
 * and the real dataset pixel */
static bool calcLeftUpperTexAbsPx() {
    uint32_t i = 0;
    Coordinate currentPosition_dc, currPosTrans;
    viewerState *viewerState = state->viewerState;

    /* why div first by mag and then multiply again with it?? */
    CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

    //iterate over all viewports
    //this function has to be called after the texture changed or the user moved, in the sense of a
    //realignment of the data
    for (i = 0; i < viewerState->numberViewPorts; i++) {
        switch (viewerState->viewPorts[i].type) {
        case VIEWPORT_XY:
            //Set the coordinate of left upper data pixel currently stored in the texture
            //viewerState->viewPorts[i].texture.usedTexLengthDc is state->M and even int.. very funny
            // this guy is always in mag1, even if a different mag dataset is active!!!
            // this might be buggy for very high mags, test that!
            SET_COORDINATE(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.y - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.z
                           * state->cubeEdgeLength * state->magnification);
    //if(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x >1000000){ LOG("uninit x %d", viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x);}
    //if(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y > 1000000){ LOG("uninit y %d", viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y);}
    //if(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z > 1000000){ LOG("uninit z %d", viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z);}

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->viewPorts[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y -
                           (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z);

            break;

        case VIEWPORT_XZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.y * state->cubeEdgeLength  * state->magnification,
                           (currentPosition_dc.z - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->viewPorts[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y ,
                           viewerState->currentPosition.z
                           - (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx));
            break;

        case VIEWPORT_YZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->viewPorts[i].texture.leftUpperPxInAbsPx,
                           currentPosition_dc.x * state->cubeEdgeLength   * state->magnification,
                           (currentPosition_dc.y - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.z - (viewerState->viewPorts[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->viewPorts[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x ,
                           viewerState->currentPosition.y
                           - (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z
                           - (int)((viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->viewPorts[i].texture.texUnitsPerDataPx));


            break;
        default:

            viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x = 0;
            viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y = 0;
            viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z = 0;
        }
    }

    return false;
}


//Initializes the viewer, is called only once after the viewing thread started
static bool initViewer() {
    /*
    calcLeftUpperTexAbsPx();

    // init the skeletonizer
    if(initSkeletonizer(state) == FALSE) {
        LOG("Error initializing the skeletonizer.");
        return FALSE;
    }

    if(SDLNet_Init() == FAIL) {
        LOG("Error initializing SDLNet: %s.", SDLNet_GetError());
        return FALSE;
    }

    // init the agar gui system
    if(initGUI() == FALSE) {
        LOG("Error initializing the agar system / gui.");
        return FALSE;
    }

    // Set up the clipboard
    if(SDLScrap_Init() < 0) {
        LOG("Couldn't init clipboard: %s\n", SDL_GetError());
        _Exit(FALSE);
    }

    // TDitem

    // Load the color map for the overlay

    if(state->overlay) {
        LOG("overlayColorMap at %p\n", &(state->viewerState->overlayColorMap[0][0]));
        if(loadDatasetColorTable("stdOverlay.lut",
                          &(state->viewerState->overlayColorMap[0][0]),
                          GL_RGBA) == FALSE) {
            LOG("Overlay color map stdOverlay.lut does not exist.");
            state->overlay = FALSE;
        }
    }

    // This is the buffer that holds the actual texture data (for _all_ textures)

    state->viewerState->texData =
            (Byte *) malloc(TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 3);
    if(state->viewerState->texData == NULL) {
        LOG("Out of memory.");
        _Exit(FALSE);
    }
    memset(state->viewerState->texData, '\0',
           TEXTURE_EDGE_LEN
           * TEXTURE_EDGE_LEN
           * sizeof(Byte)
           * 3);

    // This is the buffer that holds the actual overlay texture data (for _all_ textures)

    if(state->overlay) {
        state->viewerState->overlayData =
                (Byte *) malloc(TEXTURE_EDGE_LEN *
                   TEXTURE_EDGE_LEN *
                   sizeof(Byte) *
                   4);
        if(state->viewerState->overlayData == NULL) {
            LOG("Out of memory.");
            _Exit(FALSE);
        }
        memset(state->viewerState->overlayData, '\0',
               TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 4);
    }

    // This is the data we use when the data for the
    //   slices is not yet available (hasn't yet been loaded).

    state->viewerState->defaultTexData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                * sizeof(Byte)
                                                * 3);
    if(state->viewerState->defaultTexData == NULL) {
        LOG("Out of memory.");
        _Exit(FALSE);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);

    // Default data for the overlays
    if(state->overlay) {
        state->viewerState->defaultOverlayData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                        * sizeof(Byte)
                                                        * 4);
        if(state->viewerState->defaultOverlayData == NULL) {
            LOG("Out of memory.");
            _Exit(FALSE);
        }
        memset(state->viewerState->defaultOverlayData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                             * sizeof(Byte)
                                                             * 4);
    }

    // init the rendering system
    if(initRenderer() == FALSE) {
        qDebug("Error initializing the rendering system.");
        return FALSE;
    }

    sendLoadSignal(state->viewerState->currentPosition.x,
                   state->viewerState->currentPosition.y,
                   state->viewerState->currentPosition.z);



    return TRUE;
    */
}


/* TODO maybe no implementation is needed anymore */
static QCursor *GenCursor(char *xpm[], int xHot, int yHot) {

}
// from knossos-global.h


bool Viewer::loadDatasetColorTable(const char *path, GLuint *table, int32_t type) {

    FILE *lutFile = NULL;
        uint8_t lutBuffer[RGBA_LUTSIZE];
        int32_t readBytes = 0, i = 0;
        uint32_t size = RGB_LUTSIZE;

        // The b is for compatibility with non-UNIX systems and denotes a
        // binary file.

        LOG("Reading Dataset LUT at %s\n", path);

        lutFile = fopen(path, "rb");
        if(lutFile == NULL) {
            LOG("Unable to open Dataset LUT at %s.", path);
            return false;
        }

        if(type == GL_RGB)
            size = RGB_LUTSIZE;
        else if(type == GL_RGBA)
            size = RGBA_LUTSIZE;
        else {
            LOG("Requested color type %x does not exist.", type);
            return false;
        }

        readBytes = (int32_t)fread(lutBuffer, 1, size, lutFile);
        if(readBytes != size) {
            LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, path, size);
            if(fclose(lutFile) != 0) {
                LOG("Additionally, an error occured closing the file.");
            }
            return false;
        }

        if(fclose(lutFile) != 0) {
            LOG("Error closing LUT file.");
            return false;
        }

        if(type == GL_RGB) {
            for(i = 0; i < 256; i++) {
                table[0 * 256 + i] = (GLuint)lutBuffer[i];
                table[1 * 256 + i] = (GLuint)lutBuffer[i + 256];
                table[2 * 256 + i] = (GLuint)lutBuffer[i + 512];
            }
        }
        else if(type == GL_RGBA) {
            for(i = 0; i < 256; i++) {
                table[0 * 256 + i] = (GLuint)lutBuffer[i];
                table[1 * 256 + i] = (GLuint)lutBuffer[i + 256];
                table[2 * 256 + i] = (GLuint)lutBuffer[i + 512];
                table[3 * 256 + i] = (GLuint)lutBuffer[i + 768];
            }
        }


    return true;

}
bool Viewer::loadTreeColorTable(const char *path, float *table, int32_t type) {

    FILE *lutFile = NULL;
        uint8_t lutBuffer[RGB_LUTSIZE];
        int32_t readBytes = 0, i = 0;
        uint32_t size = RGB_LUTSIZE;

        // The b is for compatibility with non-UNIX systems and denotes a
        // binary file.
        LOG("Reading Tree LUT at %s\n", path);

        lutFile = fopen(path, "rb");
        if(lutFile == NULL) {
            LOG("Unable to open Tree LUT at %s.", path);
            return false;
        }

        if(type != GL_RGB) {
            /* AG_TextError("Tree colors only support RGB colors. Your color type is: %x", type); */
            LOG("Chosen color was of type %x, but expected GL_RGB", type);
            return false;
        }

        readBytes = (int32_t)fread(lutBuffer, 1, size, lutFile);
        if(readBytes != size) {
            LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, path, size);
            if(fclose(lutFile) != 0) {
                LOG("Additionally, an error occured closing the file.");
            }
            return false;
        }

        if(fclose(lutFile) != 0) {
            LOG("Error closing LUT file.");
            return false;
        }

        //Get RGB-Values in percent
        for(i = 0; i < 256; i++) {
            table[i]   = lutBuffer[i]/MAX_COLORVAL;
            table[i + 256] = lutBuffer[i+256]/MAX_COLORVAL;
            table[i + 512] = lutBuffer[i + 512]/MAX_COLORVAL;
        }

        GUI::treeColorAdjustmentsChanged();

    return true;
}

bool Viewer::updatePosition(int32_t serverMovement) { return true;}

bool Viewer::calcDisplayedEdgeLength() {

    int32_t i;
       float FOVinDCs;

       FOVinDCs = ((float)state->M) - 1.f;

       for(i = 0; i < state->viewerState->numberViewPorts; i++) {
           state->viewerState->viewPorts[i].texture.displayedEdgeLengthX =
           state->viewerState->viewPorts[i].texture.displayedEdgeLengthY =
               FOVinDCs * (float)state->cubeEdgeLength
               / (float) tempConfig->viewerState->viewPorts[i].texture.edgeLengthPx;
       }

       return true;

}



/* takes care of all necessary changes inside the viewer and signals
the loader to change the dataset */
/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
bool Viewer::changeDatasetMag(uint32_t upOrDownFlag) {
    uint32_t i;

        if(state->viewerState->datasetMagLock) {
            return false;
        }

        switch(upOrDownFlag) {
            case MAG_DOWN:
                if(state->magnification > state->lowestAvailableMag) {
                    state->magnification /= 2;
                    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
                        if(state->viewerState->viewPorts[i].type != VIEWPORT_SKELETON) {
                            state->viewerState->viewPorts[i].texture.zoomLevel *= 2.0;
                            upsampleVPTexture(&state->viewerState->viewPorts[i]);
                            state->viewerState->viewPorts[i].texture.texUnitsPerDataPx
                                *= 2.;
                        }
                    }
                }
                else return false;
            break;

            case MAG_UP:
                if(state->magnification < state->highestAvailableMag) {
                    state->magnification *= 2;
                    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
                        if(state->viewerState->viewPorts[i].type != VIEWPORT_SKELETON) {
                            state->viewerState->viewPorts[i].texture.zoomLevel *= 0.5;
                            downsampleVPTexture(&state->viewerState->viewPorts[i]);
                            state->viewerState->viewPorts[i].texture.texUnitsPerDataPx
                                /= 2.;
                        }
                    }
                }
                else return false;
            break;
        }

        /* necessary? */
        state->viewerState->userMove = TRUE;
        recalcTextureOffsets();

      /*  for(i = 0; i < state->viewerState->numberViewPorts; i++) {
            if(state->viewerState->viewPorts[i].type != VIEWPORT_SKELETON) {
                LOG("left upper tex px of VP %d is: %d, %d, %d",i, state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x, state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y, state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z);

            }
        }*/
       Knossos::sendDatasetChangeSignal(upOrDownFlag);
        //refreshViewports();
        /* set flags to trigger the necessary renderer updates */
        //state->skeletonState->skeletonChanged = TRUE;



    return true;
}


//Entry point for viewer thread, general viewer coordination, "main loop"
bool Viewer::viewer() { return true;}

//Initializes the window with the parameter given in viewerState
bool Viewer::createScreen() { return true;}

//Transfers all (orthogonal viewports) textures completly from ram (*viewerState->viewPorts[i].texture.data) to video memory
//Calling makes only sense after full initialization of the SDL / OGL screen
bool Viewer::initializeTextures() { return true;}

//Frees allocated memory
bool Viewer::cleanUpViewer( viewerState *viewerState) { return true;}

bool Viewer::updateViewerState() { return true;}
bool Viewer::updateZoomCube() { return true;}
bool Viewer::userMove(int32_t x, int32_t y, int32_t z, int32_t serverMovement) { return true;}
int32_t Viewer::findVPnumByWindowCoordinate(uint32_t xScreen, uint32_t yScreen) { return 0;}

bool Viewer::recalcTextureOffsets() {

    uint32_t i;
        float midX, midY;

        midX = midY = 0.;

        /* Every time the texture offset coords change,
        the skeleton VP must be updated. */
        state->skeletonState->viewChanged = TRUE;
        calcDisplayedEdgeLength();

        for(i = 0; i < state->viewerState->numberViewPorts; i++) {
            /* Do this only for orthogonal VPs... */
            if (state->viewerState->viewPorts[i].type == VIEWPORT_XY
                    || state->viewerState->viewPorts[i].type == VIEWPORT_XZ
                    || state->viewerState->viewPorts[i].type == VIEWPORT_YZ) {
                /*Don't remove /2 *2, integer division! */

                /* old code for smaller FOV
                            state->viewerState->viewPorts[i].texture.displayedEdgeLengthX =
                                state->viewerState->viewPorts[i].texture.displayedEdgeLengthY =
                                    ((float)(((state->M / 2) * 2 - 1) * state->cubeEdgeLength))
                                    / ((float)state->viewerState->viewPorts[i].texture.edgeLengthPx);

                */

                        /* new code for larger FOV */
                        /* displayedEdgeLength is in texture pixels, independent from the
                         * currently active mag! */



                //Multiply the zoom factor. (only truncation possible! 1 stands for minimal zoom)
                state->viewerState->viewPorts[i].texture.displayedEdgeLengthX *=
                    state->viewerState->viewPorts[i].texture.zoomLevel;
                state->viewerState->viewPorts[i].texture.displayedEdgeLengthY *=
                    state->viewerState->viewPorts[i].texture.zoomLevel;

                //... and for the right orthogonal VP
                switch(state->viewerState->viewPorts[i].type) {
                    case VIEWPORT_XY:
                        //Aspect ratio correction..
                        if(state->viewerState->voxelXYRatio < 1)
                            state->viewerState->viewPorts[i].texture.displayedEdgeLengthY *=
                                state->viewerState->voxelXYRatio;
                        else
                            state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /=
                                state->viewerState->voxelXYRatio;

                        //Display only entire pixels (only truncation possible!) WHY??

                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthX =
                            (float) (
                                (int) (
                                    state->viewerState->viewPorts[i].texture.displayedEdgeLengthX
                                    / 2.
                                    / state->viewerState->viewPorts[i].texture.texUnitsPerDataPx
                                )
                                * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx
                            )
                            * 2.;

                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthY =
                            (float)
                            (((int)(state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                            2. /
                            state->viewerState->viewPorts[i].texture.texUnitsPerDataPx)) *
                            state->viewerState->viewPorts[i].texture.texUnitsPerDataPx) *
                            2.;

                        // Update screen pixel to data pixel mapping values
                        state->viewerState->viewPorts[i].screenPxXPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].screenPxYPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        // Pixels on the screen per 1 unit in the data coordinate system at the
                        // original magnification. These guys appear to be unused!!! jk 14.5.12
                        /*state->viewerState->viewPorts[i].screenPxXPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxXPerDataPx *
                            state->magnification;

                        state->viewerState->viewPorts[i].screenPxYPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxYPerDataPx *
                            state->magnification;
    */
                        state->viewerState->viewPorts[i].displayedlengthInNmX =
                            state->viewerState->voxelDimX *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].displayedlengthInNmY =
                            state->viewerState->voxelDimY *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                            state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        /* scale to 0 - 1; midX is the current pos in tex coords */
                        /* leftUpperPxInAbsPx is in always in mag1, independent of
                         * the currently active mag */
                        midX = (float)(state->viewerState->currentPosition.x -
                                 state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x)
                                 // / (float)state->viewerState->viewPorts[i].texture.edgeLengthPx;
                                 * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx;

                        midY = (float)(state->viewerState->currentPosition.y -
                                 state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y)
                                 //(float)state->viewerState->viewPorts[i].texture.edgeLengthPx;
                                 * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx;

                        //Update state->viewerState->viewPorts[i].leftUpperDataPxOnScreen with this call
                        calcLeftUpperTexAbsPx();

                        //Offsets for crosshair
                        state->viewerState->viewPorts[i].texture.xOffset = ((float)(state->viewerState->currentPosition.x - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.x)) * state->viewerState->viewPorts[i].screenPxXPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxXPerDataPx;
                        state->viewerState->viewPorts[i].texture.yOffset = ((float)(state->viewerState->currentPosition.y - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.y)) * state->viewerState->viewPorts[i].screenPxYPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxYPerDataPx;

                        break;
                    case VIEWPORT_XZ:
                        //Aspect ratio correction..
                        if(state->viewerState->voxelXYtoZRatio < 1) state->viewerState->viewPorts[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                        else state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;

                        //Display only entire pixels (only truncation possible!)
                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthX = (float)(((int)(state->viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2. / state->viewerState->viewPorts[i].texture.texUnitsPerDataPx)) * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx) * 2.;
                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthY = (float)(((int)(state->viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2. / state->viewerState->viewPorts[i].texture.texUnitsPerDataPx)) * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx) * 2.;

                        //Update screen pixel to data pixel mapping values
                        state->viewerState->viewPorts[i].screenPxXPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].screenPxYPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        // Pixels on the screen per 1 unit in the data coordinate system at the
                        // original magnification.
                        state->viewerState->viewPorts[i].screenPxXPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxXPerDataPx *
                            state->magnification;

                        state->viewerState->viewPorts[i].screenPxYPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxYPerDataPx *
                            state->magnification;

                        state->viewerState->viewPorts[i].displayedlengthInNmX =
                            state->viewerState->voxelDimX *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].displayedlengthInNmY =
                            state->viewerState->voxelDimZ *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        midX = ((float)(state->viewerState->currentPosition.x - state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.x))
                               * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx; //scale to 0 - 1
                        midY = ((float)(state->viewerState->currentPosition.z - state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z))
                               * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx; //scale to 0 - 1

                        //Update state->viewerState->viewPorts[i].leftUpperDataPxOnScreen with this call
                        calcLeftUpperTexAbsPx();

                        //Offsets for crosshair
                        state->viewerState->viewPorts[i].texture.xOffset = ((float)(state->viewerState->currentPosition.x - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.x)) * state->viewerState->viewPorts[i].screenPxXPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxXPerDataPx;
                        state->viewerState->viewPorts[i].texture.yOffset = ((float)(state->viewerState->currentPosition.z - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.z)) * state->viewerState->viewPorts[i].screenPxYPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxYPerDataPx;

                        break;
                    case VIEWPORT_YZ:
                        //Aspect ratio correction..
                        if(state->viewerState->voxelXYtoZRatio < 1) state->viewerState->viewPorts[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                        else state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;

                        //Display only entire pixels (only truncation possible!)
                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthX = (float)(((int)(state->viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2. / state->viewerState->viewPorts[i].texture.texUnitsPerDataPx)) * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx) * 2.;
                        state->viewerState->viewPorts[i].texture.displayedEdgeLengthY = (float)(((int)(state->viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2. / state->viewerState->viewPorts[i].texture.texUnitsPerDataPx)) * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx) * 2.;

                        // Update screen pixel to data pixel mapping values
                        // WARNING: YZ IS ROTATED AND MIRRORED! So screenPxXPerDataPx
                        // corresponds to displayedEdgeLengthY and so on.
                        state->viewerState->viewPorts[i].screenPxXPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].screenPxYPerDataPx =
                            (float)state->viewerState->viewPorts[i].edgeLength /
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        // Pixels on the screen per 1 unit in the data coordinate system at the
                        // original magnification.
                        state->viewerState->viewPorts[i].screenPxXPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxXPerDataPx *
                            state->magnification;

                        state->viewerState->viewPorts[i].screenPxYPerOrigMagUnit =
                            state->viewerState->viewPorts[i].screenPxYPerDataPx *
                            state->magnification;

                        state->viewerState->viewPorts[i].displayedlengthInNmX =
                            state->viewerState->voxelDimZ *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        state->viewerState->viewPorts[i].displayedlengthInNmY =
                            state->viewerState->voxelDimY *
                            (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX /
                             state->viewerState->viewPorts[i].texture.texUnitsPerDataPx);

                        midX = ((float)(state->viewerState->currentPosition.y - state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.y))
                               // / (float)state->viewerState->viewPorts[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx;
                        midY = ((float)(state->viewerState->currentPosition.z - state->viewerState->viewPorts[i].texture.leftUpperPxInAbsPx.z))
                               // / (float)state->viewerState->viewPorts[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->viewPorts[i].texture.texUnitsPerDataPx;

                        //Update state->viewerState->viewPorts[i].leftUpperDataPxOnScreen with this call
                        calcLeftUpperTexAbsPx();

                        //Offsets for crosshair
                        state->viewerState->viewPorts[i].texture.xOffset = ((float)(state->viewerState->currentPosition.z - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.z)) * state->viewerState->viewPorts[i].screenPxXPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxXPerDataPx;
                        state->viewerState->viewPorts[i].texture.yOffset = ((float)(state->viewerState->currentPosition.y - state->viewerState->viewPorts[i].leftUpperDataPxOnScreen.y)) * state->viewerState->viewPorts[i].screenPxYPerDataPx + 0.5 * state->viewerState->viewPorts[i].screenPxYPerDataPx;

                        break;
                }

                //Calculate the vertices in texture coordinates
                /* mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0. */
                state->viewerState->viewPorts[i].texture.texLUx = midX - (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2.);
                state->viewerState->viewPorts[i].texture.texLUy = midY - (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2.);
                state->viewerState->viewPorts[i].texture.texRUx = midX + (state->viewerState->viewPorts[i].texture.displayedEdgeLengthX / 2.);
                state->viewerState->viewPorts[i].texture.texRUy = state->viewerState->viewPorts[i].texture.texLUy;
                state->viewerState->viewPorts[i].texture.texRLx = state->viewerState->viewPorts[i].texture.texRUx;
                state->viewerState->viewPorts[i].texture.texRLy = midY + (state->viewerState->viewPorts[i].texture.displayedEdgeLengthY / 2.);
                state->viewerState->viewPorts[i].texture.texLLx = state->viewerState->viewPorts[i].texture.texLUx;
                state->viewerState->viewPorts[i].texture.texLLy = state->viewerState->viewPorts[i].texture.texRLy;


            }
        }
        //Reload the height/width-windows in viewports
        GUI::reloadDataSizeWin();
        return true;

    return true;
}

bool Viewer::refreshViewports() {
    // TODO reimplementation due to qt
    return true;

}

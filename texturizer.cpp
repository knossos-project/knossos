#include "texturizer.h"
#include "knossos.h"

extern stateInfo *state;

Texturizer::Texturizer(QObject *parent) :
    QObject(parent)
{
    slicer = new Slicer();
}

bool Texturizer::vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.

    uint x_px = 0, x_dc = 0, y_px = 0, y_dc = 0;
    Coordinate upperLeftDc, currentDc, currentPosition_dc;
    Coordinate currPosTrans, leftUpperPxInAbsPxTrans;

    Byte *datacube = NULL, *overlayCube = NULL;
    int dcOffset = 0, index = 0;

    CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    CPY_COORDINATE(leftUpperPxInAbsPxTrans, currentVp->vpConfig->texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(leftUpperPxInAbsPxTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

    upperLeftDc = Coordinate::Px2DcCoord(leftUpperPxInAbsPxTrans);

    // We calculate the coordinate of the DC that holds the slice that makes up the upper left
    // corner of our texture.
    // dcOffset is the offset by which we can index into a datacube to extract the first byte of
    // slice relevant to the texture for this viewport.
    //
    // Rounding should be explicit!
    switch(currentVp->vpConfig->type) {
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
        qDebug("No such slice view: %d.", currentVp->vpConfig->type);
        return false;
    }
    // We iterate over the texture with x and y being in a temporary coordinate
    // system local to this texture.
    for(x_dc = 0; x_dc < currentVp->vpConfig->texture.usedTexLengthDc; x_dc++) {
        for(y_dc = 0; y_dc < currentVp->vpConfig->texture.usedTexLengthDc; y_dc++) {
            x_px = x_dc * state->cubeEdgeLength;
            y_px = y_dc * state->cubeEdgeLength;

            switch(currentVp->vpConfig->type) {
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
                LOG("No such slice type (%d) in vpGenerateTexture.", currentVp->vpConfig->type)
            }

            state->protectCube2Pointer->lock();
            datacube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            overlayCube = Hashtable::ht_get(state->Oc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();


            // Take care of the data textures.

            glBindTexture(GL_TEXTURE_2D,
                          currentVp->vpConfig->texture.texHandle);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // This is used to index into the texture. texData[index] is the first
            // byte of the datacube slice at position (x_dc, y_dc) in the texture.
            index = texIndex(x_dc, y_dc, 3, &(currentVp->vpConfig->texture));

            if(datacube == HT_FAILURE) {
                /* @todo
                backlogAddElement(currentVp->backlog,
                                  currentDc,
                                  dcOffset,
                                  &(viewerState->texData[index]),
                                  x_px,
                                  y_px,
                                  CUBE_DATA);
                */
                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                x_px,
                                y_px,
                                state->cubeEdgeLength,
                                state->cubeEdgeLength,
                                GL_RGB,
                                GL_UNSIGNED_BYTE,
                                viewerState->defaultTexData);
            } else {

                //qDebug() << "vpGenerateTexture" <<correct_cubes;
                slicer->dcSliceExtract(datacube,
                               &(viewerState->texData[index]),
                               dcOffset,
                               currentVp->vpConfig);
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

            //Take care of the overlay textures.
            if(state->overlay) {
                glBindTexture(GL_TEXTURE_2D,
                              currentVp->vpConfig->texture.overlayHandle);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                // This is used to index into the texture. texData[index] is the first
                // byte of the datacube slice at position (x_dc, y_dc) in the texture.
                index = texIndex(x_dc, y_dc, 4, &(currentVp->vpConfig->texture));

                if(overlayCube == HT_FAILURE) {
                    /* @todo
                    backlogAddElement(currentVp->backlog,
                                      currentDc,
                                      dcOffset * OBJID_BYTES,
                                      &(viewerState->overlayData[index]),
                                      x_px,
                                      y_px,
                                      CUBE_OVERLAY);
                    */
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
                    slicer->ocSliceExtract(overlayCube,
                                   &(viewerState->overlayData[index]),
                                   dcOffset * OBJID_BYTES,
                                   currentVp->vpConfig);

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

bool Texturizer::vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState) {
    vpBacklogElement *currentElement = NULL,
                     *nextElement = NULL;
    Byte *cube = NULL;
    uint elements = 0,
              i = 0;

    if(currentVp->backlog->entry == NULL) {
        qDebug("Called vpHandleBacklog, but there is no backlog.");
        return false;
    }

    elements = currentVp->backlog->elements;
    currentElement = currentVp->backlog->entry;
    qDebug() << "starting to handle backlog";
    for(i = 0; i < elements; i++)  {
        nextElement = currentElement->next;

        if(currentElement->cubeType == CUBE_DATA) {
            state->protectCube2Pointer->lock();
            qDebug() << currentElement->cube.x << " " << currentElement->cube.y << " " << currentElement->cube.z;
            cube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {
                //qDebug() << "failed to get cube in backlog";
                // if(currentElement->cube.x >= 3) {
                       //LOG("handleBL: currentDc %d, %d, %d", currentElement->cube.x, currentElement->cube.y, currentElement->cube.z)
                 //}
                 //LOG("failed to get cube in viewer")
            } else {
                slicer->dcSliceExtract(cube,
                               currentElement->slice,
                               currentElement->dcOffset,
                               currentVp->vpConfig);

                glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.texHandle);
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
                /* @todo backlogDelElement(currentVp->backlog, currentElement) */
            }
        }
        else if(currentElement->cubeType == CUBE_OVERLAY) {

            state->protectCube2Pointer->lock();
            cube = Hashtable::ht_get((Hashtable*) state->Oc2Pointer, currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {

            }
            else {
                slicer->ocSliceExtract(cube,
                               currentElement->slice,
                               currentElement->dcOffset,
                               currentVp->vpConfig);

                glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.overlayHandle);
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
                /* @todo backlogDelElement(currentVp->backlog, currentElement); */
            }
        }

        currentElement = nextElement;
    }

    if(currentVp->backlog->elements != 0) {
        return false;
    }
    else {
        return true;
    }
}

int Texturizer::texIndex(uint x, uint y, uint colorMultiplicationFactor, viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}


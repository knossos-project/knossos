/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include "viewer.h"
#include <QDebug>
#include "knossos.h"
#include "skeletonizer.h"
#include "renderer.h"
#include "widgets/widgetcontainer.h"

#include "sleeper.h"
#include "widgets/mainwindow.h"
#include "widgets/viewport.h"
#include "functions.h"
#include <qopengl.h>
#include <QtConcurrent/QtConcurrentRun>

extern stateInfo *state;

Viewer::Viewer(QObject *parent) :
    QThread(parent)
{
    window = new MainWindow();
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QDesktopWidget *desktop = QApplication::desktop();

    window->show();
    window->setGeometry(desktop->availableGeometry().center().x(), desktop->availableGeometry().center().y(), 1024, 800);
    window->adjustSize();
    window->loadSettings();
    state->console = window->widgetContainer->console;

    vp = window->viewports[0];
    vp2 = window->viewports[1];
    vp3 = window->viewports[2];
    vp4 = window->viewports[3];

    /*
    vp = new Viewport(window, VIEWPORT_XY, VIEWPORT_XY);
    vp2 = new Viewport(window, VIEWPORT_YZ, VIEWPORT_YZ);
    vp3 = new Viewport(window, VIEWPORT_XZ, VIEWPORT_XZ);
    vp4 = new Viewport(window, VIEWPORT_SKELETON, VIEWPORT_SKELETON);

    vp->setGeometry(5, window->toolBar->geometry().top() + window->toolBar->height() + 5, 350, 350);
    vp2->setGeometry(360, window->toolBar->geometry().top() + window->toolBar->height() + 5, 350, 350);
    vp3->setGeometry(5, window->toolBar->geometry().top() + window->toolBar->height() + 10 + 350, 350, 350);
    vp4->setGeometry(360, window->toolBar->geometry().top() + window->toolBar->height() + 10 + 350, 350, 350);
    state->viewerState->screenSizeY = vp4->geometry().bottom();

    vp->show();
    vp2->show();
    vp3->show();
    vp4->show();

    window->setGeometry(500, 300, 200, 200);
    */
    timer = new QTimer();

    /* order of the initialization of the rendering system is
     * 1. initViewer
     * 2. new Skeletonizer
     * 3. new Renderer
    */

    SET_COORDINATE(state->viewerState->currentPosition, 830, 1000, 830)

    initViewer();
    skeletonizer = new Skeletonizer();
    renderer = new Renderer();

    // This is needed for the viewport text rendering
    vp->delegate->reference = vp2->delegate->reference = vp3->delegate->reference = vp4->delegate->reference = renderer;
    vp->reference = vp2->reference = vp3->reference = vp4->reference = renderer;

    renderer->reference = vp;
    renderer->ref2 = vp2;
    renderer->ref3 = vp3;
    renderer->ref4 = vp4;

    rewire();
    frames = 0;

    window->setCoordinates(state->viewerState->currentPosition.x, state->viewerState->currentPosition.y, state->viewerState->currentPosition.z);

    state->alpha = 0;
    state->beta = 0;

    for (uint i = 0; i < state->viewerState->numberViewports; i++){
        state->viewerState->vpConfigs[i].s_max =  state->viewerState->vpConfigs[i].t_max = (((int)((state->M/2+1)*state->cubeEdgeLength/sqrt(2)))/2)*2;
    }

    floatCoordinate v1, v2, v3;
    getDirectionalVectors(state->alpha, state->beta, &v1, &v2, &v3);

    CPY_COORDINATE(state->viewerState->vpConfigs[0].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[0].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[0].n , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[1].v1 , v1);
    CPY_COORDINATE(state->viewerState->vpConfigs[1].v2 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[1].n , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[2].v1 , v3);
    CPY_COORDINATE(state->viewerState->vpConfigs[2].v2 , v2);
    CPY_COORDINATE(state->viewerState->vpConfigs[2].n , v1);


    connect(timer, SIGNAL(timeout()), this, SLOT(run()));
    timer->start(15);

}

vpList *Viewer::vpListNew() {
    vpList *newVpList = NULL;

    newVpList = (vpList *) malloc(sizeof(vpList));
    if(newVpList == NULL) {
        LOG("Out of memory.\n")
        return NULL;
    }

    newVpList->entry = NULL;
    newVpList->elements = 0;

    return newVpList;
}

int Viewer::vpListAddElement(vpList *vpList, vpConfig *vpConfig, vpBacklog *backlog) {
    vpListElement *newElement;

    newElement = (vpListElement *) malloc(sizeof(vpListElement));
    if(newElement == NULL) {
        qDebug("Out of memory\n");
        // Do not return false here. That's a bug. FAIL is hackish... Is there a
        // better way? */
        return FAIL;
    }

    newElement->vpConfig = vpConfig;
    newElement->backlog = backlog;

    if(vpList->entry != NULL) {
        vpList->entry->next->previous = newElement;
        newElement->next = vpList->entry->next;
        vpList->entry->next = newElement;
        newElement->previous = vpList->entry;
    }
    else {
        vpList->entry = newElement;
        vpList->entry->next = newElement;
        vpList->entry->previous = newElement;
    }

    vpList->elements = vpList->elements + 1;

    return vpList->elements;
}

vpBacklog *Viewer::backlogNew() {
    vpBacklog *newBacklog;

    newBacklog = (vpBacklog *) malloc(sizeof( vpBacklog));
    if(newBacklog == NULL) {
        LOG("Out of memory.\n")
        return NULL;
    }
    newBacklog->entry = NULL;
    newBacklog->elements = 0;

    return newBacklog;
}

vpList *Viewer::vpListGenerate(viewerState *viewerState) {
    vpList *newVpList = NULL;
    vpBacklog *currentBacklog = NULL;
    uint i = 0;

    newVpList = vpListNew();
    if(newVpList == NULL) {
        LOG("Error generating new vpList.")
        _Exit(false);
    }

    for(i = 0; i < viewerState->numberViewports; i++) {
        if(viewerState->vpConfigs[i].type == VIEWPORT_SKELETON) {
            continue;
        }
        currentBacklog = backlogNew();
        if(currentBacklog == NULL) {
            LOG("Error creating backlog.")
            _Exit(false);
        }
        vpListAddElement(newVpList, &(viewerState->vpConfigs[i]), currentBacklog);
    }
    return newVpList;
}

int Viewer::backlogDelElement(vpBacklog *backlog, vpBacklogElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        backlog->entry = NULL;
    }
    else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        backlog->entry = element->next;
    }
    free(element);

    backlog->elements = backlog->elements - 1;

    return backlog->elements;
}

pxStripeList *Viewer::stripesNew() {
    struct pxStripeList *newStripes;

    newStripes = (pxStripeList *)malloc(sizeof(struct pxStripeList));
    if(newStripes == NULL) {
        qDebug("Out of memory.\n");
        return NULL;
    }
    newStripes->entry = NULL;
    newStripes->elements = 0;

    return newStripes;
}

vpBacklogElement *Viewer::backlogAddElement_arb(vpBacklog *backlog, Coordinate dataCube, uint cubeType) {
    /* @arb */
    struct vpBacklogElement *newElement;

    newElement = (vpBacklogElement *)malloc(sizeof(struct vpBacklogElement));
    if(newElement == NULL) {
        LOG("Out of memory.")
        // Do not return false here. That's a bug. FAIL is hackish... Is there a better way?
        return NULL;
    }

    newElement->slice = NULL;
    SET_COORDINATE(newElement->cube, dataCube.x, dataCube.y, dataCube.z);
    newElement->x_px = 0;
    newElement->y_px = 0;
    newElement->dcOffset = 0;
    newElement->cubeType = cubeType;

    newElement->stripes = NULL;

    newElement->stripes = stripesNew();
    if(newElement->stripes == NULL) {
        LOG("Error creating stripeList.")
        exit(0);
    }

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

    return newElement;

}



bool Viewer::pxStripeListDel(pxStripeList *stripes) {

    while(stripes->elements > 0) {
           if(pxStripeListDelElement(stripes, stripes->entry) < 0) {
               LOG("Error deleting stripe element at %p from the slot stripelist %d elements remain in the list.",
                      stripes->entry, stripes->elements)
               return false;
           }
       }

    free(stripes);

    return true;
}

bool Viewer::pxStripeListDelElement(pxStripeList *stripes, pxStripe *stripe) {
    stripes->entry = stripe->next;


    free(stripe);

    stripes->elements = stripes->elements - 1;

    return stripes->elements;

}

bool Viewer::addPxStripe(vpBacklogElement *backlogElement, floatCoordinate *currentPxInDc_float, uint s, uint t1, uint t2) {
    struct pxStripe *newStripe;

    newStripe = (pxStripe *)malloc(sizeof(struct pxStripe));
    if(newStripe == NULL) {
        LOG("Out of memory.")
        /* Do not return false here. That's a bug. FAIL is hackish... Is there a better way? */
        return false;
    }

    newStripe->s = s;
    newStripe->t1 = t1;
    newStripe->t2 = t2;

    CPY_COORDINATE(newStripe->currentPxInDc_float, (*currentPxInDc_float));

    if(backlogElement == 0) {
        qDebug() << " NOOOO";
    } else {
        qDebug() << backlogElement;
        qDebug() << "YEEES";
    }

    if(backlogElement && backlogElement->stripes == 0) {
        qDebug() << "Fall 1";
    } else if(backlogElement && backlogElement->stripes)
        qDebug() << " Fall 2";

    if(backlogElement->stripes->entry != 0) {
        qDebug() << "FOOOO";
    }


    newStripe->next = backlogElement->stripes->entry;
    backlogElement->stripes->entry = newStripe;

    backlogElement->stripes->elements += 1;

    return backlogElement->stripes->elements;
}


vpBacklogElement *Viewer::isCubeInBacklog(struct vpBacklog *backlog, Coordinate *cube) {

    struct vpBacklogElement *blElement = NULL;
     /*@arb */
    if (backlog->elements != 0){
        blElement = backlog->entry;
        for (uint i = 0; i<backlog->elements; i++) {
            if (COMPARE_COORDINATE(blElement->cube,(*cube))) return blElement;
            blElement = blElement->next;
        }
        return blElement;
    }
    else
    return NULL;
}

bool Viewer::backlogDel(vpBacklog *backlog) {
    while(backlog->elements > 0) {
        if(backlogDelElement(backlog, backlog->entry) < 0) {
            LOG("Error deleting element at %p from the backlog. %d elements remain in the list.",
                backlog->entry, backlog->elements)
            return false;
        }
    }

    free(backlog);

    return true;
}


int Viewer::vpListDelElement(vpList *list,  vpListElement *element) {
    if(element->next == element) {
        // This is the only element in the list
        list->entry = NULL;
    } else {
        element->next->previous = element->previous;
        element->previous->next = element->next;
        list->entry = element->next;
    }

    if(backlogDel(element->backlog) == false) {
        LOG("Error deleting backlog at %p of vpList element at %p.",
               element->backlog, element);
        return FAIL;
    }
    free(element);

    list->elements = list->elements - 1;

    return list->elements;
}



bool Viewer::vpListDel(vpList *list) {
    while(list->elements > 0) {
        if(vpListDelElement(list, list->entry) < 0) {
            LOG("Error deleting element at %p from the slot list %d elements remain in the list.",
                   list->entry, list->elements);
            return false;
        }
    }

    return true;
}

int Viewer::backlogAddElement(vpBacklog *backlog, Coordinate datacube, uint dcOffset,
                                 Byte *slice, uint x_px, uint y_px, uint cubeType) {
    vpBacklogElement *newElement;

    newElement = (vpBacklogElement *) malloc(sizeof( vpBacklogElement));
    if(newElement == NULL) {
        LOG("Out of memory.")
        /* Do not return false here. That's a bug. FAIL is hackish... Is there a better way? */
        return FAIL;
    }

    newElement->slice = slice;
    newElement->cube = datacube;
    newElement->x_px = x_px;
    newElement->y_px = y_px;
    newElement->dcOffset = dcOffset;
    newElement->cubeType = cubeType;

    if(backlog->entry != NULL) {
        backlog->entry->next->previous = newElement;
        newElement->next = backlog->entry->next;
        backlog->entry->next = newElement;
        newElement->previous = backlog->entry;
    }
    else {
        backlog->entry = newElement;
        backlog->entry->next = newElement;
        backlog->entry->previous = newElement;
    }
    backlog->elements = backlog->elements + 1;
    return backlog->elements;
}

bool Viewer::resetViewPortData(vpConfig *viewport) {
    /* @arb */
    memset(viewport->viewPortData, state->viewerState->defaultTexData[0], TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
    viewport->s_max = viewport->t_max  = -1;
    /**/
    return true;
}

bool Viewer::sliceExtract_standard(Byte *datacube,
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

bool Viewer::sliceExtract_standard_arb(Byte *datacube, struct vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t) {
    /* @arb */
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(viewPort->v2);

    SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float->x)),
                                  (roundFloat(currentPxInDc_float->y)),
                                  (roundFloat(currentPxInDc_float->z)) );

    while((0 <= currentPxInDc.x && currentPxInDc.x < state->cubeEdgeLength) && (0 <= currentPxInDc.y && currentPxInDc.y < state->cubeEdgeLength) && (0 <= currentPxInDc.z && currentPxInDc.z < state->cubeEdgeLength)){

        //sliceIndex = 3 * ( (s + viewPort->x_offset) + (*t + viewPort->y_offset)  *  state->cubeEdgeLength * state->M);
        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);
        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        slice[sliceIndex] = slice[sliceIndex + 1]
                          = slice[sliceIndex + 2]
                          = datacube[dcIndex];

        (*t)++;
        if ((*t)>=(viewPort->t_max)) break;
        ADD_COORDINATE( (*currentPxInDc_float), (*v2));
        SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float->x)),
                                      (roundFloat(currentPxInDc_float->y)),
                                      (roundFloat(currentPxInDc_float->z)) );

        }
        /**/

        return true;
}

bool Viewer::sliceExtract_standard_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float, int s, int t1, int t2) {

    Byte *slice = viewPort->viewPortData;

    Coordinate currentPxInDc;

    floatCoordinate currentPxInDc_float;

    int sliceIndex = 0, dcIndex = 0;

    floatCoordinate *v2 = &(viewPort->v2);



    CPY_COORDINATE(currentPxInDc_float, (*startPxInDc_float))

    SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float.x)),

                                  (roundFloat(currentPxInDc_float.y)),

                                  (roundFloat(currentPxInDc_float.z)) );



    while(t1<=t2){

        SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float.x)),

                                      (roundFloat(currentPxInDc_float.y)),

                                      (roundFloat(currentPxInDc_float.z)) );

        //sliceIndex = 3 * ( (s + viewPort->x_offset) + (t1 + viewPort->y_offset)  *  state->cubeEdgeLength * state->M);

        sliceIndex = 3 * ( s + t1  *  state->cubeEdgeLength * state->M);

        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;

        slice[sliceIndex] = slice[sliceIndex + 1]

                          = slice[sliceIndex + 2]

                          = datacube[dcIndex];



        t1++;

        ADD_COORDINATE( (currentPxInDc_float), (*v2));

    }


    return true;
}


bool Viewer::sliceExtract_adjust(Byte *datacube,
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

bool Viewer::sliceExtract_adjust_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t) {
    /* @arb */
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    int sliceIndex = 0, dcIndex = 0;
    floatCoordinate *v2 = &(viewPort->v2);

    SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float->x)),
                                  (roundFloat(currentPxInDc_float->y)),
                                  (roundFloat(currentPxInDc_float->z)));

    while((0 <= currentPxInDc.x && currentPxInDc.x < state->cubeEdgeLength) && (0 <= currentPxInDc.y && currentPxInDc.y < state->cubeEdgeLength) && (0 <= currentPxInDc.z && currentPxInDc.z < state->cubeEdgeLength)){

        //sliceIndex = 3 * ( (s + viewPort->x_offset) + (*t + viewPort->y_offset)  *  state->cubeEdgeLength * state->M);
        sliceIndex = 3 * ( s + *t  *  state->cubeEdgeLength * state->M);

        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        slice[sliceIndex] = state->viewerState->datasetAdjustmentTable[0][datacube[dcIndex]];
        slice[sliceIndex + 1] = state->viewerState->datasetAdjustmentTable[1][datacube[dcIndex]];
        slice[sliceIndex + 2] = state->viewerState->datasetAdjustmentTable[2][datacube[dcIndex]];

        (*t)++;
        if ((*t)>=(viewPort->t_max)) break;
        ADD_COORDINATE( (*currentPxInDc_float), (*v2));
        SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float->x)),
                                      (roundFloat(currentPxInDc_float->y)),
                                      (roundFloat(currentPxInDc_float->z)) );

    }
    /**/

        return true;
}

bool Viewer::sliceExtract_adjust_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float, int s, int t1, int t2) {
    Byte *slice = viewPort->viewPortData;
    Coordinate currentPxInDc;
    floatCoordinate currentPxInDc_float;
    int sliceIndex = 0, dcIndex = 0;
    //floatCoordinate *v1 = &(viewPort->v1);
    floatCoordinate *v2 = &(viewPort->v2);

    CPY_COORDINATE(currentPxInDc_float, (*startPxInDc_float))
    SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float.x)),
                                  (roundFloat(currentPxInDc_float.y)),
                                  (roundFloat(currentPxInDc_float.z)) );

    while(t1<=t2){
        SET_COORDINATE(currentPxInDc, (roundFloat(currentPxInDc_float.x)),
                                      (roundFloat(currentPxInDc_float.y)),
                                      (roundFloat(currentPxInDc_float.z)) );
        //sliceIndex = 3 * ( (s + viewPort->x_offset) + (t1 + viewPort->y_offset)  *  state->cubeEdgeLength * state->M);
        sliceIndex = 3 * ( s + t1  *  state->cubeEdgeLength * state->M);
        dcIndex = currentPxInDc.x + currentPxInDc.y * state->cubeEdgeLength + currentPxInDc.z * state->cubeSliceArea;
        slice[sliceIndex] = state->viewerState->datasetAdjustmentTable[0][datacube[dcIndex]];
        slice[sliceIndex + 1] = state->viewerState->datasetAdjustmentTable[1][datacube[dcIndex]];
        slice[sliceIndex + 2] = state->viewerState->datasetAdjustmentTable[2][datacube[dcIndex]];

        t1++;
        ADD_COORDINATE( (currentPxInDc_float), (*v2));
    }
}


bool Viewer::dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
    datacube += dcOffset;

    if(state->viewerState->datasetAdjustmentOn) {
        /* Texture type GL_RGB and we need to adjust coloring */
        //QFuture<bool> future = QtConcurrent::run(this, &Viewer::sliceExtract_adjust, datacube, slice, vpConfig);
        //future.waitForFinished();
        sliceExtract_adjust(datacube, slice, vpConfig);
    }
    else {
        /* Texture type GL_RGB and we don't need to adjust anything*/
       //QFuture<bool> future = QtConcurrent::run(this, &Viewer::sliceExtract_adjust, datacube, slice, vpConfig);
       //future.waitForFinished();
       sliceExtract_standard(datacube, slice, vpConfig);
    }
    return true;
}

bool Viewer::dcSliceExtract_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *currentPxInDc_float, int s, int *t) {
    /* @arb */
    if(state->viewerState->datasetAdjustmentOn) {
        // Texture type GL_RGB and we need to adjust coloring
        sliceExtract_adjust_arb(datacube, viewPort, currentPxInDc_float, s, t);
    } else {
        // Texture type GL_RGB and we don't need to adjust anything
        sliceExtract_standard_arb(datacube, viewPort, currentPxInDc_float, s, t);
    } /**/
        return true;
}

bool Viewer::dcSliceExtract_Backlog_arb(Byte *datacube, vpConfig *viewPort, floatCoordinate *startPxInDc_float, int s, int t1, int t2) {
    /* @arb */
    if(state->viewerState->datasetAdjustmentOn) {
        // Texture type GL_RGB and we need to adjust coloring
        sliceExtract_adjust_Backlog_arb(datacube, viewPort, startPxInDc_float, s, t1, t2);
    }
    else {
        // Texture type GL_RGB and we don't need to adjust anything
        sliceExtract_standard_Backlog_arb(datacube, viewPort, startPxInDc_float, s, t1, t2);
    }
    /**/
    return true;
}

bool Viewer::ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig) {
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

bool Viewer::vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState) {
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
    //LOG("starting to handle backlog")
    for(i = 0; i < elements; i++)  {
        nextElement = currentElement->next;

        if(currentElement->cubeType == CUBE_DATA) {
            state->protectCube2Pointer->lock();
            //qDebug() << currentElement->cube.x << " " << currentElement->cube.y << " " << currentElement->cube.z;
            cube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {
                //qDebug("failed to get cube in backlog");

            } else {
                dcSliceExtract(cube,
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
                backlogDelElement(currentVp->backlog, currentElement);
            }
        }
        else if(currentElement->cubeType == CUBE_OVERLAY) {

            state->protectCube2Pointer->lock();
            cube = Hashtable::ht_get((Hashtable*) state->Oc2Pointer, currentElement->cube);
            state->protectCube2Pointer->unlock();

            if(cube == HT_FAILURE) {

            }
            else {
                ocSliceExtract(cube,
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
                backlogDelElement(currentVp->backlog, currentElement);
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

bool Viewer::vpHandleBacklog_arb(struct vpListElement *currentVp, struct viewerState *viewerState) {
    struct vpBacklogElement *currentElement = NULL,
                                *nextElement = NULL;

        /* @arb */
        struct pxStripe *stripe;
        Byte *cube = NULL;
        uint elements = 0, i = 0, j = 0;

        if(currentVp->backlog->entry == NULL) {
            //LOG("Called vpHandleBacklog, but there is no backlog.")
            return false;
        }

        elements = currentVp->backlog->elements;
        currentElement = currentVp->backlog->entry;

        glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.texHandle);

        for(i = 0; i < elements; i++)  {
            nextElement = currentElement->next;
            if(currentElement->cubeType == CUBE_DATA) {
                state->protectCube2Pointer->lock();
                cube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentElement->cube);
                state->protectCube2Pointer->unlock();


                if(cube == HT_FAILURE) {                   
                } else {

                    stripe = currentElement->stripes->entry;
                    for (j = 0; j < currentElement->stripes->elements; j++){

                        dcSliceExtract_Backlog_arb(cube,
                                                   currentVp->vpConfig,
                                                   &stripe->currentPxInDc_float,
                                                   stripe->s,
                                                   stripe->t1,
                                                   stripe->t2);
                        stripe = stripe->next;

                    }

                    backlogDelElement(currentVp->backlog, currentElement);
                }
            }
            else if(currentElement->cubeType == CUBE_OVERLAY) {
            }

            currentElement = nextElement;
        }

         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, state->M*state->cubeEdgeLength, state->M*state->cubeEdgeLength, GL_RGB, GL_UNSIGNED_BYTE, currentVp->vpConfig->viewPortData);

        glBindTexture(GL_TEXTURE_2D, 0);

        if(currentVp->backlog->elements != 0)
            return false;
        else
            return true;
        /**/
}

static int texIndex(uint x,
                        uint y,
                        uint colorMultiplicationFactor,
                        viewportTexture *texture) {
    uint index = 0;

    index = x * state->cubeSliceArea + y
            * (texture->edgeLengthDc * state->cubeSliceArea)
            * colorMultiplicationFactor;

    return index;
}

bool Viewer::vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState) {
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
                qDebug("No such slice type (%d) in vpGenerateTexture.", currentVp->vpConfig->type);
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
            } else {

                //qDebug() << "vpGenerateTexture" <<correct_cubes;
                dcSliceExtract(datacube,
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

bool Viewer::vpGenerateTexture_arb(struct vpListElement *currentVp, struct viewerState *viewerState) {
    // Load the texture for a viewport by going through all relevant datacubes and copying slices
    // from those cubes into the texture.


    Coordinate lowerRightPxInDc, upperLeftPxInDc, currentDc, currentPx;
    floatCoordinate currentPxInDc_float, rowPx_float, currentPx_float;

    uint t_old = 0;
    Byte *datacube = NULL, *overlayCube = NULL;

    struct vpBacklogElement *backlogElement = NULL;

    floatCoordinate *v1 = &(currentVp->vpConfig->v1);
    floatCoordinate *v2 = &(currentVp->vpConfig->v2);

    CPY_COORDINATE(rowPx_float, currentVp->vpConfig->texture.leftUpperPxInAbsPx);
    DIV_COORDINATE(rowPx_float, state->magnification);
    CPY_COORDINATE(currentPx_float, rowPx_float);


    glBindTexture(GL_TEXTURE_2D, currentVp->vpConfig->texture.texHandle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int s = 0;
    int t = 0;

    while(s < (currentVp->vpConfig->s_max)){
        t = 0;
        while(t < (currentVp->vpConfig->t_max)){

            SET_COORDINATE(currentPx, roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z));
            SET_COORDINATE(currentDc, currentPx.x/state->cubeEdgeLength,
                                      currentPx.y/state->cubeEdgeLength,
                                      currentPx.z/state->cubeEdgeLength);

            if (currentPx.x < 0) currentDc.x -=1;
            if (currentPx.y < 0) currentDc.y -=1;
            if (currentPx.z < 0) currentDc.z -=1;


            state->protectCube2Pointer->lock();
            datacube = Hashtable::ht_get(state->Dc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            overlayCube = Hashtable::ht_get(state->Oc2Pointer[Knossos::log2uint32(state->magnification)], currentDc);
            state->protectCube2Pointer->unlock();


            backlogElement = isCubeInBacklog(currentVp->backlog, &currentDc);

            if (backlogElement != NULL) {
               SET_COORDINATE(currentPxInDc_float, (currentPx_float.x-currentDc.x*state->cubeEdgeLength),
                                                    (currentPx_float.y-currentDc.y*state->cubeEdgeLength),
                                                    (currentPx_float.z-currentDc.z*state->cubeEdgeLength ));

                //special case: if currentPx_float.x-currentDc.x*state->cubeEdgeLength) == -0.5 then currentPxInDc_float.x would be 127.5 which would be rounded to 128 <--- not good, should be 127 in this case,
                //otherwise it would result in a closed loop

                if ( currentPxInDc_float.x == 127.5 ) currentPxInDc_float.x-=0.001;

                if ( currentPxInDc_float.y == 127.5 ) currentPxInDc_float.y-=0.001;

                if ( currentPxInDc_float.z == 127.5 ) currentPxInDc_float.z-=0.001;

                if ( currentPxInDc_float.x == -0.5 ) currentPxInDc_float.x+=0.001;

                if ( currentPxInDc_float.y == -0.5 ) currentPxInDc_float.y+=0.001;

                if ( currentPxInDc_float.z == -0.5 ) currentPxInDc_float.z+=0.001;

                SET_COORDINATE(upperLeftPxInDc, (currentDc.x*state->cubeEdgeLength),
                                                (currentDc.y*state->cubeEdgeLength),
                                                (currentDc.z*state->cubeEdgeLength));

                SET_COORDINATE(lowerRightPxInDc, ((currentDc.x+1)*state->cubeEdgeLength-1),
                                                 ((currentDc.y+1)*state->cubeEdgeLength-1),
                                                 ((currentDc.z+1)*state->cubeEdgeLength-1) );

                SET_COORDINATE(currentPx, roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z));

                t_old = t;

                while(CONTAINS_COORDINATE(currentPx, upperLeftPxInDc, lowerRightPxInDc)) {
                    ADD_COORDINATE(currentPx_float, (*v2));
                    SET_COORDINATE(currentPx, roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z));
                    t++;

                    if (t>=currentVp->vpConfig->t_max) break;

                }

                qDebug() << "px 1";
                this->addPxStripe(backlogElement, &currentPxInDc_float, s, t_old, t-1);

            }

            else if(datacube == HT_FAILURE) {

                backlogElement = backlogAddElement_arb(currentVp->backlog,currentDc,CUBE_DATA);

                SET_COORDINATE(currentPxInDc_float, (currentPx_float.x-currentDc.x*state->cubeEdgeLength),

                                                    (currentPx_float.y-currentDc.y*state->cubeEdgeLength),

                                                    (currentPx_float.z-currentDc.z*state->cubeEdgeLength ));              

                if ( currentPxInDc_float.x == 127.5 ) currentPxInDc_float.x-=0.001;

                if ( currentPxInDc_float.y == 127.5 ) currentPxInDc_float.y-=0.001;

                if ( currentPxInDc_float.z == 127.5 ) currentPxInDc_float.z-=0.001;

                if ( currentPxInDc_float.x == -0.5 ) currentPxInDc_float.x+=0.001;

                if ( currentPxInDc_float.y == -0.5 ) currentPxInDc_float.y+=0.001;

                if ( currentPxInDc_float.z == -0.5 ) currentPxInDc_float.z+=0.001;



                SET_COORDINATE(upperLeftPxInDc, (currentDc.x*state->cubeEdgeLength),
                                             (currentDc.y*state->cubeEdgeLength),
                                                (currentDc.z*state->cubeEdgeLength));

                SET_COORDINATE(lowerRightPxInDc, ((currentDc.x+1)*state->cubeEdgeLength - 1),
                                                 ((currentDc.y+1)*state->cubeEdgeLength - 1),

                                                 ((currentDc.z+1)*state->cubeEdgeLength - 1));

                SET_COORDINATE(currentPx, roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z));

                t_old = t;

                while(CONTAINS_COORDINATE(currentPx, upperLeftPxInDc, lowerRightPxInDc)){
                    ADD_COORDINATE(currentPx_float, (*v2));
                    SET_COORDINATE(currentPx, roundFloat(currentPx_float.x), roundFloat(currentPx_float.y), roundFloat(currentPx_float.z));
                    t++;
                    if (t>=currentVp->vpConfig->t_max) break;
                }

                qDebug() << "px 2";
                addPxStripe(backlogElement, &currentPxInDc_float, s, t_old, t-1);

            }
            else {

                SET_COORDINATE(currentPxInDc_float, (currentPx_float.x-currentDc.x*state->cubeEdgeLength),

                                                    (currentPx_float.y-currentDc.y*state->cubeEdgeLength),

                                                    (currentPx_float.z-currentDc.z*state->cubeEdgeLength));

                //special case: if currentPx_float.x-currentDc.x*state->cubeEdgeLength) == -0.5 then currentPxInDc_float.x would be 127.5 which would be rounded to 128 <--- not good, should be 127 in this case,

                //otherwise it would result in a closed loop

                if ( currentPxInDc_float.x == 127.5 ) currentPxInDc_float.x-=0.001;

                if ( currentPxInDc_float.y == 127.5 ) currentPxInDc_float.y-=0.001;

                if ( currentPxInDc_float.z == 127.5 ) currentPxInDc_float.z-=0.001;

                if ( currentPxInDc_float.x == -0.5 ) currentPxInDc_float.x+=0.001;

                if ( currentPxInDc_float.y == -0.5 ) currentPxInDc_float.y+=0.001;

                if ( currentPxInDc_float.z == -0.5 ) currentPxInDc_float.z+=0.001;

                t_old = t;


                dcSliceExtract_arb(datacube,
                                   currentVp->vpConfig,

                                   &currentPxInDc_float,

                                   s, &t);

                SET_COORDINATE(currentPx_float, (currentPx_float.x + v2->x * (t-t_old)),

                                                (currentPx_float.y + v2->y * (t-t_old)),

                                                (currentPx_float.z + v2->z * (t-t_old)) );

            }


             //  Take care of the overlay textures.


            if(state->overlay) {

            }
        }

        s++;

        ADD_COORDINATE(rowPx_float, (*v1) );
        CPY_COORDINATE(currentPx_float, rowPx_float)
    }

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    state->M*state->cubeEdgeLength,
                    state->M*state->cubeEdgeLength,
                    GL_RGB,
                    GL_UNSIGNED_BYTE,
                    currentVp->vpConfig->viewPortData);


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

static bool downsampleVPTexture(vpConfig *vpConfig) {
    /* allocate 2 texture buffers */
    //Byte *orig, *resampled;

    /* get the texture */

    /* downsample it */

    /* send it to the graphicscard again */

    return true;
}


static bool upsampleVPTexture(vpConfig *vpConfig) {
   return true;
}

/* this function calculates the mapping between the left upper texture pixel
 * and the real dataset pixel */
bool Viewer::calcLeftUpperTexAbsPx() {
    uint i = 0;
    Coordinate currentPosition_dc, currPosTrans;
    viewerState *viewerState = state->viewerState;

    /* why div first by mag and then multiply again with it?? */
    CPY_COORDINATE(currPosTrans, viewerState->currentPosition);
    DIV_COORDINATE(currPosTrans, state->magnification);

    currentPosition_dc = Coordinate::Px2DcCoord(currPosTrans);

    //iterate over all viewports
    //this function has to be called after the texture changed or the user moved, in the sense of a
    //realignment of the data
    for (i = 0; i < viewerState->numberViewports; i++) {
        switch (viewerState->vpConfigs[i].type) {
        case VIEWPORT_XY:
            //Set the coordinate of left upper data pixel currently stored in the texture
            //viewerState->vpConfigs[i].texture.usedTexLengthDc is state->M and even int.. very funny
            // this guy is always in mag1, even if a different mag dataset is active!!!
            // this might be buggy for very high mags, test that!
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.y - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.z
                           * state->cubeEdgeLength * state->magnification);
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x >1000000){ LOG("uninit x %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x)}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y > 1000000){ LOG("uninit y %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y)}
            //if(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z > 1000000){ LOG("uninit z %d", viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z)}

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y -
                           (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z);
            break;
        case VIEWPORT_XZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           (currentPosition_dc.x - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           currentPosition_dc.y * state->cubeEdgeLength  * state->magnification,
                           (currentPosition_dc.z - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y ,
                           viewerState->currentPosition.z
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));
            break;
        case VIEWPORT_YZ:
            //Set the coordinate of left upper data pixel currently stored in the texture
            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           currentPosition_dc.x * state->cubeEdgeLength   * state->magnification,
                           (currentPosition_dc.y - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification,
                           (currentPosition_dc.z - (viewerState->vpConfigs[i].texture.usedTexLengthDc / 2))
                           * state->cubeEdgeLength * state->magnification);

            //Set the coordinate of left upper data pixel currently displayed on screen
            //The following lines are dependent on the current VP orientation, so rotation of VPs messes that
            //stuff up! A more general solution would be better.
            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           viewerState->currentPosition.x ,
                           viewerState->currentPosition.y
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z
                           - (int)((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));
            break;
        case VIEWPORT_ARBITRARY:
            floatCoordinate v1, v2;
            /* @arb */

            CPY_COORDINATE(v1, viewerState->vpConfigs[i].v1);
            CPY_COORDINATE(v2, viewerState->vpConfigs[i].v2);
            SET_COORDINATE(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float,
                           viewerState->currentPosition.x - v1.x * viewerState->vpConfigs[i].s_max/2 - v2.x * viewerState->vpConfigs[i].t_max/2,
                           viewerState->currentPosition.y - v1.y * viewerState->vpConfigs[i].s_max/2 - v2.y * viewerState->vpConfigs[i].t_max/2,
                           viewerState->currentPosition.z - v1.z * viewerState->vpConfigs[i].s_max/2 - v2.z * viewerState->vpConfigs[i].t_max/2);

            SET_COORDINATE(viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx,
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.x)* state->magnification,
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.y)* state->magnification,
                           roundFloat(viewerState->vpConfigs[i].leftUpperPxInAbsPx_float.z)* state->magnification);

            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen_float,
                           viewerState->currentPosition.x
                           - v1.x * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)

                           - v2.x * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.y

                           - v1.y * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                           - v2.y * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx),
                           viewerState->currentPosition.z

                           - v1.z * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                           - v2.z * ((viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.)
                            / viewerState->vpConfigs[i].texture.texUnitsPerDataPx));

            SET_COORDINATE(state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen,
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x),
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y),
                           roundFloat(viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z));
            /**/
            break;
        default:
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x = 0;
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y = 0;
            viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z = 0;
        }
    }
    return false;
}

/**
  * Initializes the viewer, is called only once after the viewing thread started
  *
  */
bool Viewer::initViewer() {

    qDebug() << "Viewer: initViewer begin";
    calcLeftUpperTexAbsPx();



    if(state->overlay) {
        LOG("overlayColorMap at %p\n", &(state->viewerState->overlayColorMap[0][0]))
        if(loadDatasetColorTable("stdOverlay.lut",
                          &(state->viewerState->overlayColorMap[0][0]),
                          GL_RGBA) == false) {
            LOG("Overlay color map stdOverlay.lut does not exist.")
            state->overlay = false;
        }
    }

    // This is the buffer that holds the actual texture data (for _all_ textures)

    state->viewerState->texData =
            (Byte *) malloc(TEXTURE_EDGE_LEN
               * TEXTURE_EDGE_LEN
               * sizeof(Byte)
               * 3);
    if(state->viewerState->texData == NULL) {
        LOG("Out of memory.")
        _Exit(false);
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
            LOG("Out of memory.")
            _Exit(false);
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
        LOG("Out of memory.")
        _Exit(false);
    }
    memset(state->viewerState->defaultTexData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                     * sizeof(Byte)
                                                     * 3);

    /* @arb */
    for(int i = 0; i < state->viewerState->numberViewports; i++){
        state->viewerState->vpConfigs[i].viewPortData = (Byte *)malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
        if(state->viewerState->vpConfigs[i].viewPortData == NULL) {
            LOG("Out of memory.")
            exit(0);
        }
        memset(state->viewerState->vpConfigs[i].viewPortData, state->viewerState->defaultTexData[0], TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN * sizeof(Byte) * 3);
    }


    // Default data for the overlays
    if(state->overlay) {
        state->viewerState->defaultOverlayData = (Byte *) malloc(TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                                 * sizeof(Byte)
                                                                 * 4);
        if(state->viewerState->defaultOverlayData == NULL) {
            LOG("Out of memory.")
            _Exit(false);
        }
        memset(state->viewerState->defaultOverlayData, '\0', TEXTURE_EDGE_LEN * TEXTURE_EDGE_LEN
                                                             * sizeof(Byte)
                                                             * 4);
    }

    updateViewerState();
    recalcTextureOffsets();

    return true;
}



// from knossos-global.h
bool Viewer::loadDatasetColorTable(QString path, GLuint *table, int type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGBA_LUTSIZE];
    uint readBytes = 0, i = 0;
    uint size = RGB_LUTSIZE;

    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.

    const char *cpath = path.toStdString().c_str();

    LOG("Reading Dataset LUT at %s\n", cpath);

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Dataset LUT at %s.", cpath)
        return false;
    }

    if(type == GL_RGB) {
        size = RGB_LUTSIZE;
    }
    else if(type == GL_RGBA) {
        size = RGBA_LUTSIZE;
    }
    else {
        LOG("Requested color type %x does not exist.", type)
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size)
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.")
        }
        return false;
    }

    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.")
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

bool Viewer::loadTreeColorTable(QString path, float *table, int type) {
    FILE *lutFile = NULL;
    uint8_t lutBuffer[RGB_LUTSIZE];
    uint readBytes = 0, i = 0;
    uint size = RGB_LUTSIZE;

    const char *cpath = path.toStdString().c_str();
    // The b is for compatibility with non-UNIX systems and denotes a
    // binary file.
    LOG("Reading Tree LUT at %s\n", cpath)

    lutFile = fopen(cpath, "rb");
    if(lutFile == NULL) {
        LOG("Unable to open Tree LUT at %s.", cpath)
        return false;
    }
    if(type != GL_RGB) {
        /* AG_TextError("Tree colors only support RGB colors. Your color type is: %x", type); */
        LOG("Chosen color was of type %x, but expected GL_RGB", type)
        return false;
    }

    readBytes = (uint)fread(lutBuffer, 1, size, lutFile);
    if(readBytes != size) {
        LOG("Could read only %d bytes from LUT file %s. Expected %d bytes", readBytes, cpath, size)
        if(fclose(lutFile) != 0) {
            LOG("Additionally, an error occured closing the file.")
        }
        return false;
    }
    if(fclose(lutFile) != 0) {
        LOG("Error closing LUT file.")
        return false;
    }

    //Get RGB-Values in percent
    for(i = 0; i < 256; i++) {
        table[i]   = lutBuffer[i]/MAX_COLORVAL;
        table[i + 256] = lutBuffer[i+256]/MAX_COLORVAL;
        table[i + 512] = lutBuffer[i + 512]/MAX_COLORVAL;
    }

    window->treeColorAdjustmentsChanged();
    return true;
}

bool Viewer::updatePosition(int serverMovement) {
    Coordinate jump;

    return true;
}

bool Viewer::calcDisplayedEdgeLength() {
    uint i;
    float FOVinDCs;

    FOVinDCs = ((float)state->M) - 1.f;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
            FOVinDCs * (float)state->cubeEdgeLength
            / (float) state->viewerState->vpConfigs[i].texture.edgeLengthPx;
    }
    return true;
}


/**
* takes care of all necessary changes inside the viewer and signals
* the loader to change the dataset
*/
/* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
bool Viewer::changeDatasetMag(uint upOrDownFlag) {
    uint i;

    if(state->viewerState->datasetMagLock) {
        return false;
    }

    switch(upOrDownFlag) {
    case MAG_DOWN:
        if(state->magnification > state->lowestAvailableMag) {
            state->magnification /= 2;
            for(i = 0; i < state->viewerState->numberViewports; i++) {
                if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel *= 2.0;
                    upsampleVPTexture(&state->viewerState->vpConfigs[i]);
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx *= 2.;
                }
            }
        }
        else return false;
        break;

    case MAG_UP:
        if(state->magnification < state->highestAvailableMag) {
            state->magnification *= 2;
            for(i = 0; i < state->viewerState->numberViewports; i++) {
                if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
                    state->viewerState->vpConfigs[i].texture.zoomLevel *= 0.5;
                    downsampleVPTexture(&state->viewerState->vpConfigs[i]);
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx /= 2.;
                }
            }
        }
        else return false;
        break;
    }

    /* necessary? */
    state->viewerState->userMove = true;
    recalcTextureOffsets();

    /*for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != VIEWPORT_SKELETON) {
            LOG("left upper tex px of VP %d is: %d, %d, %d",i,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y,
                state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z)
        }
    }*/
    sendLoadSignal(state->viewerState->currentPosition.x,
                            state->viewerState->currentPosition.y,
                            state->viewerState->currentPosition.z,
                            upOrDownFlag);

    /* set flags to trigger the necessary renderer updates */
    state->skeletonState->skeletonChanged = true;
    skeletonizer->skeletonChanged = true;

    emit updateZoomAndMultiresWidgetSignal();

    return true;
}

/**
  * @brief This method is used for the Viewer-Thread declared in main(knossos.cpp)
  * This is the old viewer() function from KNOSSOS 3.2
  * It works as follows:
  * - The viewer get initialized
  * - The rendering loop starts:
  *   - lists of pending texture parts are iterated and loaded if they are available
  *   - If not they are added to a backlog which is processed at a later time.
  *   - TODO: The Eventhandling in QT works asnyc, new concept are currently in progress
  * - the loadSignal occurs in three different locations:
  *   - initViewer
  *   - changeDatasetMag
  *   - userMove
  *
  */
//Entry point for viewer thread, general viewer coordination, "main loop"
void Viewer::run() {    

    QTime bench;


    // Event and rendering loop.
    // What happens is that we go through lists of pending texture parts and load
    // them if they are available. If they aren't, they are added to a backlog
    // which is processed at a later time.
    // While we are loading the textures, we check for events. Some events
    // might cancel the current loading process. When all textures / backlogs
    // have been processed, we go into an idle state, in which we wait for events.
    //qDebug() << "Viewer: start begin";
    struct viewerState *viewerState = state->viewerState;
    //struct vpList *viewports = NULL;
    struct vpListElement *currentVp = NULL, *nextVp = NULL;
    uint drawCounter = 0;

    state->viewerState->viewerReady = true;

    // Display info about skeleton save path here TODO
    // This creates a circular doubly linked list of
        // pending viewports (viewports for which the texture has not yet been
        // completely loaded) from the viewport-array in the viewerState
        // structure.
        // The idea is that we can easily remove the element representing a
        // pending viewport once its texture is completely loaded.
        viewports = vpListGenerate(viewerState);
        currentVp = viewports->entry;

        state->alpha += state->viewerState->alphaCache;
        state->beta+= state->viewerState->betaCache;
        state->viewerState->alphaCache = state->viewerState->betaCache = 0;

        if (state->alpha >= 360)
            state->alpha -= 360;
        else if (state->alpha < 0)
            state->alpha += 360;
        if (state->beta >= 360)
            state->beta -= 360;
        else if (state->beta < 0)
            state->beta += 360;

        getDirectionalVectors(state->alpha, state->beta, &v1, &v2, &v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[0].v1 , v1);
        CPY_COORDINATE(state->viewerState->vpConfigs[0].v2 , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[0].n , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[1].v1 , v1);
        CPY_COORDINATE(state->viewerState->vpConfigs[1].v2 , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[1].n , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[2].v1 , v3);
        CPY_COORDINATE(state->viewerState->vpConfigs[2].v2 , v2);
        CPY_COORDINATE(state->viewerState->vpConfigs[2].n , v1);
        recalcTextureOffsets();


        while(viewports->elements > 0) {

            if(drawCounter == 0) {
                vp->makeCurrent();               
            } else if(drawCounter == 1) {
                vp2->makeCurrent();                
            } else if(drawCounter == 2) {
                vp3->makeCurrent();                
            }

            nextVp = currentVp->next;
            // printf("currentVp at %p, nextVp at %p.\n", currentVp, nextVp);

            // We iterate over the list and either handle the backlog (a list
            // of datacubes and associated offsets, see headers) if there is
            // one or start loading everything from scratch if there is none.

            if(currentVp->vpConfig->type != VIEWPORT_SKELETON) {

                currentVp->backlog->elements = 0;

                if(currentVp->backlog->elements == 0) {
                    // There is no backlog. That means we haven't yet attempted
                    // to load the texture for this viewport, which is what we
                    // do now. If we can't complete the texture because a Dc
                    // is missing, a backlog is generated.
                    //vpGenerateTexture(currentVp, viewerState);
                    /* @arb */

                    if(currentVp->vpConfig->type != VIEWPORT_ARBITRARY)
                         vpGenerateTexture(currentVp, viewerState);
                    else
                        vpGenerateTexture_arb(currentVp, viewerState);



                } else {
                    // There is a backlog. We go through its elements
                    //vpHandleBacklog(currentVp, viewerState);
                    /* @arb */

                    if (currentVp->vpConfig->type != VIEWPORT_ARBITRARY)
                        vpHandleBacklog(currentVp, viewerState);
                    else
                        vpHandleBacklog_arb(currentVp, viewerState);

                }

                if(currentVp->backlog->elements == 0) {
                    // There is no backlog after either handling the backlog
                    // or loading the whole texture. That means the texture is
                    // complete. We can remove the viewport/ from the list.

                    //  XXX TODO XXX
                    //  The Dc2Pointer hashtable locking is currently done at pretty high
                    //  frequency by vpHandleBacklog() and might slow down the
                    //  loader.
                    //  We might want to introduce a locked variable that says how many
                    //  yet "unused" (by the viewer) cubes the loader has provided.
                    //  Unfortunately, we can't non-busy wait on the loader _and_
                    //  events, unless the loader generates events itself... So if this
                    //  really is a bottleneck it might be worth to think about it
                    //  again.
                    vpListDelElement(viewports, currentVp);
                }
            }

            drawCounter++;
            if(drawCounter == 3) {
                drawCounter = 0;

                updateViewerState();
                recalcTextureOffsets();
                skeletonizer->updateSkeletonState();

                vp->updateGL();
                vp2->updateGL();
                vp3->updateGL();
                vp4->updateGL();

                return;

            }

            // An incoming user movement event makes the current backlog &
            // viewport lists obsolete and we regenerate them dependant on
            // the new position

            // Leaves the loop that checks regularily for new available
            // texture parts because the whole texture has to be recreated
            // if the users moves

            currentVp = nextVp;

        }//end while(viewports->elements > 0)
        vpListDel(viewports);

        viewerState->userMove = false;
        frames += 4;
        logSingle();
        //qDebug() << frames << " frames";

        //qDebug() << bench.elapsed() << " ms";
}

void Viewer::logSingle() {
    if(state->singleLogging) {

        viewportTexture tex = state->viewerState->vpConfigs[0].texture;

        QPlainTextEdit *editor = new QPlainTextEdit();
        editor->insertPlainText(QString("DEBUGGING TEXTURE DATA\n"));
        editor->insertPlainText(QString("texHandle: %1\n").arg(tex.texHandle));
        editor->insertPlainText(QString("overlayHandle: %1\n").arg(tex.overlayHandle));
        editor->insertPlainText(QString("leftUpperPxInAbsPx: %1, %2, %3\n").arg(tex.leftUpperPxInAbsPx.x).arg(tex.leftUpperPxInAbsPx.y).arg(tex.leftUpperPxInAbsPx.z));
        editor->insertPlainText(QString("edgeLengthDC: %1\n").arg(tex.edgeLengthDc));
        editor->insertPlainText(QString("edgeLengthPx: %1\n").arg(tex.edgeLengthPx));
        editor->insertPlainText(QString("displayedEdgeLengthX: %1\n").arg(tex.displayedEdgeLengthX));
        editor->insertPlainText(QString("displayedEdgeLengthY: %1\n").arg(tex.displayedEdgeLengthY));
        editor->insertPlainText(QString("texUnitsPerDataPx: %1\n").arg(tex.texUnitsPerDataPx));

        editor->insertPlainText(QString("texLUx: %1\n").arg(tex.texLUx));
        editor->insertPlainText(QString("texLUy: %1\n").arg(tex.texLUy));
        editor->insertPlainText(QString("texLLx: %1\n").arg(tex.texLLx));
        editor->insertPlainText(QString("texLLy: %1\n").arg(tex.texLLy));
        editor->insertPlainText(QString("texRUx: %1\n").arg(tex.texRUx));
        editor->insertPlainText(QString("texRUy: %1\n").arg(tex.texRUy));
        editor->insertPlainText(QString("texRLx: %1\n").arg(tex.texRLx));
        editor->insertPlainText(QString("texRLy: %1\n").arg(tex.texRLy));

        editor->insertPlainText(QString("xOffset: %1\n").arg(tex.xOffset));
        editor->insertPlainText(QString("yOffset: %1\n").arg(tex.yOffset));
        editor->insertPlainText(QString("zoomLevel: %1\n").arg(tex.zoomLevel));

        editor->show();
        state->singleLogging = false;
    }
}

void Viewer::showFrames() {
    qDebug() << "frames :" << frames;
    frames = 0;
}

bool Viewer::updateViewerState() {

   uint i;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(i == 0)
            vp->makeCurrent();
        else if(i == 1)
            vp2->makeCurrent();
        else if(i == 2)
            vp3->makeCurrent();

        glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
        // Set the parameters for the texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);
        glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.overlayHandle);
        // Set the parameters for the texture.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->viewerState->filterType);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state->viewerState->filterType);

    }
    glBindTexture(GL_TEXTURE_2D, 0);

    updateZoomCube();

    return true;
}


bool Viewer::updateZoomCube() {
    uint i;
    int residue, max, currentZoomCube, oldZoomCube;

    /* Notice int division! */
    max = ((state->M/2)*2-1);
    oldZoomCube = state->viewerState->zoomCube;
    state->viewerState->zoomCube = 0;

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if(state->viewerState->vpConfigs[i].type != (uint)VIEWPORT_SKELETON) {
            residue = ((max*state->cubeEdgeLength)
            - ((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)))
            / state->cubeEdgeLength;

            if(residue%2) {
                residue = residue / 2 + 1;
            }
            else if((residue%2 == 0) && (residue != 0)) {
                residue = (residue - 1) / 2 + 1;
            }
            currentZoomCube = (state->M/2)-residue;
            if(state->viewerState->zoomCube < currentZoomCube) {
                state->viewerState->zoomCube = currentZoomCube;
            }
            residue = ((max*state->cubeEdgeLength)
            - ((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)))
            / state->cubeEdgeLength;

            if(residue%2) {
                residue = residue / 2 + 1;
            }
            else if((residue%2 == 0) && (residue != 0)) {
                residue = (residue - 1) / 2 + 1;
            }
            currentZoomCube = (state->M/2)-residue;
            if(state->viewerState->zoomCube < currentZoomCube) {
                state->viewerState->zoomCube = currentZoomCube;
            }
        }
    }
    if(oldZoomCube != state->viewerState->zoomCube) {
        state->skeletonState->skeletonChanged = true;
        //skeletonizer->skeletonChanged = true;
    }
    return true;
}

bool Viewer::userMove(int x, int y, int z, int serverMovement) {
    //qDebug() << frames << " t1";
    struct viewerState *viewerState = state->viewerState;

    Coordinate lastPosition_dc;
    Coordinate newPosition_dc;

    //The skeleton VP view has to be updated after a current pos change
    state->skeletonState->viewChanged = true;
    skeletonizer->viewChanged = true;
    /* @todo case decision for skeletonizer */
    if(state->skeletonState->showIntersections) {
        state->skeletonState->skeletonSliceVPchanged = true;
    }
    // This determines whether the server will broadcast the coordinate change
    // to its client or not.

    lastPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition);

    viewerState->userMove = true;


    if ((viewerState->currentPosition.x + x) >= 0 &&
        (viewerState->currentPosition.x + x) <= state->boundary.x &&
        (viewerState->currentPosition.y + y) >= 0 &&
        (viewerState->currentPosition.y + y) <= state->boundary.y &&
        (viewerState->currentPosition.z + z) >= 0 &&
        (viewerState->currentPosition.z + z) <= state->boundary.z) {
            viewerState->currentPosition.x += x;
            viewerState->currentPosition.y += y;
            viewerState->currentPosition.z += z;
            SET_COORDINATE(state->currentDirections[state->currentDirectionsIndex], x, y, z);
            state->currentDirectionsIndex = (state->currentDirectionsIndex + 1) % LL_CURRENT_DIRECTIONS_SIZE;
    }
    else {
        LOG("Position (%d, %d, %d) out of bounds",
            viewerState->currentPosition.x + x + 1,
            viewerState->currentPosition.y + y + 1,
            viewerState->currentPosition.z + z + 1)
    }

    //qDebug() << state->viewerState->currentPosition.x << " " << state->viewerState->currentPosition.y;

    calcLeftUpperTexAbsPx();
    recalcTextureOffsets();
    newPosition_dc = Coordinate::Px2DcCoord(viewerState->currentPosition);

    if(serverMovement == TELL_COORDINATE_CHANGE &&
        state->clientState->connected == true &&
        state->clientState->synchronizePosition) {
        emit broadcastPosition(viewerState->currentPosition.x,
                                  viewerState->currentPosition.y,
                                  viewerState->currentPosition.z);
    }


    if(!COMPARE_COORDINATE(newPosition_dc, lastPosition_dc)) {
        state->viewerState->superCubeChanged = true;

        sendLoadSignal(viewerState->currentPosition.x,
                                viewerState->currentPosition.y,
                                viewerState->currentPosition.z,
                                NO_MAG_CHANGE);
    }

    QtConcurrent::run(this, &Viewer::updateCoordinatesSignal, viewerState->currentPosition.x, viewerState->currentPosition.y, viewerState->currentPosition.z);
    emit idleTimeSignal();

    return true;
}

bool Viewer::userMove_arb(float x, float y, float z, int serverMovement) {
    /* @arb */
    Coordinate step;
    state->viewerState->moveCache.x += x;
    state->viewerState->moveCache.y += y;
    state->viewerState->moveCache.z += z;
    step.x = roundFloat(state->viewerState->moveCache.x);
    step.y = roundFloat(state->viewerState->moveCache.y);
    step.z = roundFloat(state->viewerState->moveCache.z);
    SUB_COORDINATE(state->viewerState->moveCache, step);
    return userMove(step.x, step.y, step.z, serverMovement);
    /**/
    return false;

}


int Viewer::findVPnumByWindowCoordinate(uint xScreen, uint yScreen) {
    uint tempNum;

    tempNum = -1;
   /* TDitem
    for(i = 0; i < state->viewerState->numberViewports; i++) {
        if((xScreen >= state->viewerState->vpConfigs[i].lowerLeftCorner.x) && (xScreen <= (state->viewerState->vpConfigs[i].lowerLeftCorner.x + state->viewerState->vpConfigs[i].edgeLength))) {
            if((yScreen >= (((state->viewerState->vpConfigs[i].lowerLeftCorner.y - state->viewerState->screenSizeY) * -1) - state->viewerState->vpConfigs[i].edgeLength)) && (yScreen <= ((state->viewerState->vpConfigs[i].lowerLeftCorner.y - state->viewerState->screenSizeY) * -1))) {
                //Window coordinate lies in that VP
                tempNum = i;
            }
        }
    }
    //The VP on top (if there are multiple VPs on this coordinate) or -1 is returned.
    */
    return tempNum;
}

bool Viewer::recalcTextureOffsets() {
    uint i;
    float midX, midY;
    floatCoordinate *v1, *v2;

    midX = midY = 0.;

    /* Every time the texture offset coords change,
    the skeleton VP must be updated. */
    state->skeletonState->viewChanged = true;
    //skeletonizer->viewChanged = true;
    calcDisplayedEdgeLength();

    for(i = 0; i < state->viewerState->numberViewports; i++) {
        /* Do this only for orthogonal VPs... */
        if (state->viewerState->vpConfigs[i].type == VIEWPORT_XY
            || state->viewerState->vpConfigs[i].type == VIEWPORT_XZ
            || state->viewerState->vpConfigs[i].type == VIEWPORT_YZ
            || state->viewerState->vpConfigs[i].type == VIEWPORT_ARBITRARY) {

            //Multiply the zoom factor. (only truncation possible! 1 stands for minimal zoom)
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX *=
                state->viewerState->vpConfigs[i].texture.zoomLevel;
            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *=
                state->viewerState->vpConfigs[i].texture.zoomLevel;

            //... and for the right orthogonal VP
            switch(state->viewerState->vpConfigs[i].type) {
            case VIEWPORT_XY:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *=
                        state->viewerState->voxelXYRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /=
                        state->viewerState->voxelXYRatio;
                }
                //Display only entire pixels (only truncation possible!) WHY??

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                    (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                  / 2.
                                  / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                            * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                    * 2.;

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                    (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                   / 2.
                                   / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                            * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                    * 2.;

                // Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    /  state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification. These guys appear to be unused!!! jk 14.5.12
                //state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                //    state->viewerState->vpConfigs[i].screenPxXPerDataPx *
                //    state->magnification;

                //state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                //    state->viewerState->vpConfigs[i].screenPxYPerDataPx *
                //    state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    state->viewerState->voxelDimX
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    state->viewerState->voxelDimY
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // scale to 0 - 1; midX is the current pos in tex coords
                // leftUpperPxInAbsPx is in always in mag1, independent of
                // the currently active mag
                midX = (float)(state->viewerState->currentPosition.x
                       - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x)
                    // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx;
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                midY = (float)(state->viewerState->currentPosition.y
                       - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y)
                     //(float)state->viewerState->vpConfigs[i].texture.edgeLengthPx;
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset =
                    ((float)(state->viewerState->currentPosition.x
                    - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x))
                    * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                    + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;

                state->viewerState->vpConfigs[i].texture.yOffset =
                    ((float)(state->viewerState->currentPosition.y
                    - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y))
                    * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                    + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

                break;
            case VIEWPORT_XZ:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYtoZRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
                }
                //Display only entire pixels (only truncation possible!)
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;

                //Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength
                    / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification.
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxXPerDataPx
                    * state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxYPerDataPx
                    * state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    state->viewerState->voxelDimX
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    state->viewerState->voxelDimZ
                    * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                midX = ((float)(state->viewerState->currentPosition.x
                               - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.x))
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx; //scale to 0 - 1
                midY = ((float)(state->viewerState->currentPosition.z
                               - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z))
                       * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx; //scale to 0 - 1

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset = ((float)(state->viewerState->currentPosition.x - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.x)) * state->viewerState->vpConfigs[i].screenPxXPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;
                state->viewerState->vpConfigs[i].texture.yOffset = ((float)(state->viewerState->currentPosition.z - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z)) * state->viewerState->vpConfigs[i].screenPxYPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

                break;
            case VIEWPORT_YZ:
                //Aspect ratio correction..
                if(state->viewerState->voxelXYtoZRatio < 1) {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= state->viewerState->voxelXYtoZRatio;
                }
                else {
                    state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= state->viewerState->voxelXYtoZRatio;
                }

                //Display only entire pixels (only truncation possible!)
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;
                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                        (float)(((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                                       / 2.
                                       / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx))
                                * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)
                        * 2.;

                // Update screen pixel to data pixel mapping values
                // WARNING: YZ IS ROTATED AND MIRRORED! So screenPxXPerDataPx
                // corresponds to displayedEdgeLengthY and so on.
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                        (float)state->viewerState->vpConfigs[i].edgeLength
                        / (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                        // Pixels on the screen per 1 unit in the data coordinate system at the
                        // original magnification.
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                        state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        * state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                        state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        * state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                        state->viewerState->voxelDimZ
                        * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                        state->viewerState->voxelDimY
                        * (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                        / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                midX = ((float)(state->viewerState->currentPosition.y
                                - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.y))
                               // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;
                midY = ((float)(state->viewerState->currentPosition.z
                                - state->viewerState->vpConfigs[i].texture.leftUpperPxInAbsPx.z))
                               // / (float)state->viewerState->vpConfigs[i].texture.edgeLengthPx; //scale to 0 - 1
                               * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;

                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset =
                        ((float)(state->viewerState->currentPosition.z
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.z))
                        * state->viewerState->vpConfigs[i].screenPxXPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;

                state->viewerState->vpConfigs[i].texture.yOffset =
                        ((float)(state->viewerState->currentPosition.y
                                 - state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen.y))
                        * state->viewerState->vpConfigs[i].screenPxYPerDataPx
                        + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;
                break;
            case VIEWPORT_ARBITRARY:
                //v1: vector in Viewport x-direction, parameter s corresponds to v1

                //v2: vector in Viewport y-direction, parameter t corresponds to v2

                /* @arb */
                state->viewerState->vpConfigs[i].s_max =  state->viewerState->vpConfigs[i].t_max = (((int)((state->M/2+1)*state->cubeEdgeLength/sqrt(2)))/2)*2;

                v1 = &(state->viewerState->vpConfigs[i].v1);

                v2 = &(state->viewerState->vpConfigs[i].v2);
                //Aspect ratio correction..



                //Calculation of new Ratio V1 to V2, V1 is along x

//                   float voxelV1V2Ratio;

//                    voxelV1V2Ratio =  sqrtf((powf(state->viewerState->voxelXYtoZRatio * state->viewerState->voxelXYRatio * v1->x, 2)

//                                     +  powf(state->viewerState->voxelXYtoZRatio * v1->y, 2) + powf(v1->z, 2))

//                                     / (powf(state->viewerState->voxelXYtoZRatio * state->viewerState->voxelXYRatio * v2->x, 2)

//                                     +  powf(state->viewerState->voxelXYtoZRatio * v2->y, 2) + powf(v2->z, 2)));

//                    if(voxelV1V2Ratio < 1)
//                        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY *= voxelV1V2Ratio;
//                    else
//                        state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= voxelV1V2Ratio;





                float voxelV1X, voxelV2X;

                voxelV1X = sqrtf((powf(v1->x, 2) +  powf(v1->y / state->viewerState->voxelXYRatio, 2)

                                  + powf(v1->z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2)));

                voxelV2X = sqrtf((powf(v2->x, 2) +  powf(v2->y / state->viewerState->voxelXYRatio, 2)

                                  + powf(v2->z / state->viewerState->voxelXYRatio / state->viewerState->voxelXYtoZRatio , 2)));

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /= voxelV1X;

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /= voxelV2X;

                //Display only entire pixels (only truncation possible!) WHY??

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX =
                    (float) (
                        (int) (
                            state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                            / 2.
                            / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                        )
                        * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                    )
                    * 2.;

                state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY =
                    (float)
                    (((int)(state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /
                    2. /
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx)) *
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx) *
                    2.;

                // Update screen pixel to data pixel mapping values
                state->viewerState->vpConfigs[i].screenPxXPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength /
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].screenPxYPerDataPx =
                    (float)state->viewerState->vpConfigs[i].edgeLength /
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                // Pixels on the screen per 1 unit in the data coordinate system at the
                // original magnification. These guys appear to be unused!!! jk 14.5.12
                state->viewerState->vpConfigs[i].screenPxXPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxXPerDataPx *
                    state->magnification;

                state->viewerState->vpConfigs[i].screenPxYPerOrigMagUnit =
                    state->viewerState->vpConfigs[i].screenPxYPerDataPx *
                    state->magnification;

                state->viewerState->vpConfigs[i].displayedlengthInNmX =
                    sqrtf(powf(state->viewerState->voxelDimX * v1->x,2) + powf(state->viewerState->voxelDimY * v1->y,2) + powf(state->viewerState->voxelDimZ * v1->z,2) ) *
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX /
                     state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);

                state->viewerState->vpConfigs[i].displayedlengthInNmY =
                    sqrtf(powf(state->viewerState->voxelDimX * v2->x,2) + powf(state->viewerState->voxelDimY * v2->y,2) + powf(state->viewerState->voxelDimZ * v2->z,2) ) *
                    (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY /
                    state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx);


                midX = state->viewerState->vpConfigs[i].s_max/2  * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;
                midY = state->viewerState->vpConfigs[i].t_max/2  * state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx;


                //Update state->viewerState->vpConfigs[i].leftUpperDataPxOnScreen with this call
                calcLeftUpperTexAbsPx();

                //Offsets for crosshair
                state->viewerState->vpConfigs[i].texture.xOffset = (midX - state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX/2) / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx * state->viewerState->vpConfigs[i].screenPxXPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxXPerDataPx;
                state->viewerState->vpConfigs[i].texture.yOffset = (midY - state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY/2) / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx * state->viewerState->vpConfigs[i].screenPxYPerDataPx + 0.5 * state->viewerState->vpConfigs[i].screenPxYPerDataPx;

                break;
            }

            //Calculate the vertices in texture coordinates
            // mid really means current pos inside the texture, in texture coordinates, relative to the texture origin 0., 0.
            state->viewerState->vpConfigs[i].texture.texLUx =
                    midX - (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.);
            state->viewerState->vpConfigs[i].texture.texLUy =
                    midY - (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.);
            state->viewerState->vpConfigs[i].texture.texRUx =
                    midX + (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX / 2.);
            state->viewerState->vpConfigs[i].texture.texRUy = state->viewerState->vpConfigs[i].texture.texLUy;
            state->viewerState->vpConfigs[i].texture.texRLx = state->viewerState->vpConfigs[i].texture.texRUx;
            state->viewerState->vpConfigs[i].texture.texRLy =
                    midY + (state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY / 2.);
            state->viewerState->vpConfigs[i].texture.texLLx = state->viewerState->vpConfigs[i].texture.texLUx;
            state->viewerState->vpConfigs[i].texture.texLLy = state->viewerState->vpConfigs[i].texture.texRLy;
        }
    }
    //Reload the height/width-windows in viewports
    MainWindow::reloadDataSizeWin();
    return true;
}

bool Viewer::sendLoadSignal(uint x, uint y, uint z, int magChanged) {
    qDebug() << "Viewer: sendLoadSignal begin";
    state->protectLoadSignal->lock();
    state->loadSignal = true;  
    state->datasetChangeSignal = magChanged;

    state->previousPositionX = state->currentPositionX;

    // Convert the coordinate to the right mag. The loader
    // is agnostic to the different dataset magnifications.
    // The int division is hopefully not too much of an issue here
    SET_COORDINATE(state->currentPositionX,
                   x / state->magnification,
                   y / state->magnification,
                   z / state->magnification);

    /*
    emit loadSignal();
    qDebug("I send a load signal to %i, %i, %i", x, y, z);
    */
    state->protectLoadSignal->unlock();

    state->conditionLoadSignal->wakeOne();
    qDebug() << "Viewer: sendLoadSignal ended";
    return true;
}

bool Viewer::moveVPonTop(uint currentVP) {

}

/** Global interfaces  */
void Viewer::rewire() {

    connect(window, SIGNAL(userMoveSignal(int, int, int, int)), this, SLOT(userMove(int,int,int,int)));
    connect(window, SIGNAL(updateCommentsTableSignal()), window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(this->vp->delegate, SIGNAL(updateCommentsTable()), window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(this->vp2->delegate, SIGNAL(updateCommentsTable()), window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(this->vp3->delegate, SIGNAL(updateCommentsTable()), window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));
    connect(this->vp4->delegate, SIGNAL(updateCommentsTable()), window->widgetContainer->commentsWidget->nodeCommentsTab, SLOT(updateCommentsTable()));

    connect(this, SIGNAL(updateZoomAndMultiresWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));

    connect(vp->delegate, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));
    connect(vp2->delegate, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));
    connect(vp3->delegate, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));

    connect(vp->delegate, SIGNAL(userMoveArbSignal(float,float,float,int)), this, SLOT(userMove_arb(float,float,float,int)));
    connect(vp2->delegate, SIGNAL(userMoveArbSignal(float,float,float,int)), this, SLOT(userMove_arb(float,float,float,int)));
    connect(vp3->delegate, SIGNAL(userMoveArbSignal(float,float,float,int)), this, SLOT(userMove_arb(float,float,float,int)));
    connect(vp4->delegate, SIGNAL(userMoveArbSignal(float,float,float,int)), this, SLOT(userMove_arb(float,float,float,int)));

    connect(vp->delegate, SIGNAL(zoomOrthoSignal(float)), vp, SLOT(zoomOrthogonals(float)));
    connect(vp2->delegate, SIGNAL(zoomOrthoSignal(float)), vp2, SLOT(zoomOrthogonals(float)));
    connect(vp3->delegate, SIGNAL(zoomOrthoSignal(float)), vp3, SLOT(zoomOrthogonals(float)));
    connect(vp4->delegate, SIGNAL(zoomOrthoSignal(float)), vp4, SLOT(zoomOrthogonals(float)));

    connect(vp->delegate, SIGNAL(updateZoomWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp2->delegate, SIGNAL(updateZoomWidgetSignal()),window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp3->delegate, SIGNAL(updateZoomWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp4->delegate, SIGNAL(updateZoomWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));

    connect(vp, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vp2, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vp3, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    connect(vp4, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));

    connect(vp, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vp2, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vp3, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(vp4, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));

    connect(vp->delegate, SIGNAL(pasteCoordinateSignal()), window, SLOT(pasteClipboardCoordinates()));
    connect(vp2->delegate, SIGNAL(pasteCoordinateSignal()), window, SLOT(pasteClipboardCoordinates()));
    connect(vp3->delegate, SIGNAL(pasteCoordinateSignal()), window, SLOT(pasteClipboardCoordinates()));
    connect(vp4->delegate, SIGNAL(pasteCoordinateSignal()), window, SLOT(pasteClipboardCoordinates()));

    connect(vp->delegate, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));
    connect(vp2->delegate, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));
    connect(vp3->delegate, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));
    connect(vp4->delegate, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));

    connect(vp->delegate, SIGNAL(updateTreeCountSignal()), window->widgetContainer->toolsWidget, SLOT(updateTreeCount()));
    connect(vp2->delegate, SIGNAL(updateTreeCountSignal()), window->widgetContainer->toolsWidget, SLOT(updateTreeCount()));
    connect(vp3->delegate, SIGNAL(updateTreeCountSignal()), window->widgetContainer->toolsWidget, SLOT(updateTreeCount()));
    connect(vp4->delegate, SIGNAL(updateTreeCountSignal()), window->widgetContainer->toolsWidget, SLOT(updateTreeCount()));

    connect(this, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));

    connect(window, SIGNAL(changeDatasetMagSignal(uint)), this, SLOT(changeDatasetMag(uint)));
    connect(window, SIGNAL(recalcTextureOffsetsSignal()), this, SLOT(recalcTextureOffsets()));
    /* @todo check *///connect(window, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));

    connect(window, SIGNAL(updateToolsSignal()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));
    connect(window, SIGNAL(saveSkeletonSignal(QString)), skeletonizer, SLOT(saveXmlSkeleton(QString)));
    connect(window, SIGNAL(loadSkeletonSignal(QString)), skeletonizer, SLOT(loadXmlSkeleton(QString)));
    connect(window, SIGNAL(updateTreeColorsSignal()), skeletonizer, SLOT(updateTreeColors()));

    connect(window, SIGNAL(stopRenderTimerSignal()), timer, SLOT(stop()));
    connect(window, SIGNAL(startRenderTimerSignal(int)), timer, SLOT(start(int)));

    connect(vp->delegate, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));
    connect(vp2->delegate, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));
    connect(vp3->delegate, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));
    connect(vp4->delegate, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));

    connect(this, SIGNAL(updateCoordinatesSignal(int,int,int)), window, SLOT(updateCoordinateBar(int,int,int)));

    connect(vp->delegate, SIGNAL(updateWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp2->delegate, SIGNAL(updateWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp3->delegate, SIGNAL(updateWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));
    connect(vp4->delegate, SIGNAL(updateWidgetSignal()), window->widgetContainer->zoomAndMultiresWidget, SLOT(update()));

    connect(vp->delegate, SIGNAL(workModeAddSignal()), window, SLOT(addNodeSlot()));
    connect(vp->delegate, SIGNAL(workModeLinkSignal()), window, SLOT(linkWithActiveNodeSlot()));
    connect(vp2->delegate, SIGNAL(workModeAddSignal()), window, SLOT(addNodeSlot()));
    connect(vp2->delegate, SIGNAL(workModeLinkSignal()), window, SLOT(linkWithActiveNodeSlot()));
    connect(vp3->delegate, SIGNAL(workModeAddSignal()), window, SLOT(addNodeSlot()));
    connect(vp3->delegate, SIGNAL(workModeLinkSignal()), window, SLOT(linkWithActiveNodeSlot()));
    connect(vp4->delegate, SIGNAL(workModeAddSignal()), window, SLOT(addNodeSlot()));
    connect(vp4->delegate, SIGNAL(workModeLinkSignal()), window, SLOT(linkWithActiveNodeSlot()));

    connect(vp->delegate, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));
    connect(vp2->delegate, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));
    connect(vp3->delegate, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));
    connect(vp4->delegate, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));

    connect(vp->delegate, SIGNAL(genTestNodesSignal(uint)), skeletonizer, SLOT(genTestNodes(uint)));
    connect(vp2->delegate, SIGNAL(genTestNodesSignal(uint)), skeletonizer, SLOT(genTestNodes(uint)));
    connect(vp3->delegate, SIGNAL(genTestNodesSignal(uint)), skeletonizer, SLOT(genTestNodes(uint)));
    connect(vp4->delegate, SIGNAL(genTestNodesSignal(uint)), skeletonizer, SLOT(genTestNodes(uint)));

    connect(vp->delegate, SIGNAL(addSkeletonNodeSignal(Coordinate*,Byte)), skeletonizer, SLOT(UI_addSkeletonNode(Coordinate*,Byte)));
    connect(vp2->delegate, SIGNAL(addSkeletonNodeSignal(Coordinate*,Byte)), skeletonizer, SLOT(UI_addSkeletonNode(Coordinate*,Byte)));
    connect(vp3->delegate, SIGNAL(addSkeletonNodeSignal(Coordinate*,Byte)), skeletonizer, SLOT(UI_addSkeletonNode(Coordinate*,Byte)));
    connect(vp4->delegate, SIGNAL(addSkeletonNodeSignal(Coordinate*,Byte)), skeletonizer, SLOT(UI_addSkeletonNode(Coordinate*,Byte)));

    connect(vp->delegate, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(vp2->delegate, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(vp3->delegate, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(vp4->delegate, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));

    connect(vp->delegate, SIGNAL(nextCommentlessNodeSignal()), skeletonizer, SLOT(nextCommentlessNode()));
    connect(vp2->delegate, SIGNAL(nextCommentlessNodeSignal()), skeletonizer, SLOT(nextCommentlessNode()));
    connect(vp3->delegate, SIGNAL(nextCommentlessNodeSignal()), skeletonizer, SLOT(nextCommentlessNode()));
    connect(vp4->delegate, SIGNAL(nextCommentlessNodeSignal()), skeletonizer, SLOT(nextCommentlessNode()));

    connect(vp->delegate, SIGNAL(previousCommentlessNodeSignal()), skeletonizer, SLOT(previousCommentlessNode()));
    connect(vp2->delegate, SIGNAL(previousCommentlessNodeSignal()), skeletonizer, SLOT(previousCommentlessNode()));
    connect(vp3->delegate, SIGNAL(previousCommentlessNodeSignal()), skeletonizer, SLOT(previousCommentlessNode()));
    connect(vp4->delegate, SIGNAL(previousCommentlessNodeSignal()), skeletonizer, SLOT(previousCommentlessNode()));

    connect(vp->delegate, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(vp2->delegate, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(vp3->delegate, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(vp4->delegate, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));

    connect(vp->delegate, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(vp2->delegate, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(vp3->delegate, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(vp4->delegate, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));

    connect(window->widgetContainer->toolsWidget, SIGNAL(setActiveTreeSignal(int)), skeletonizer, SLOT(setActiveTreeByID(int)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(lockPositionSignal(Coordinate)), skeletonizer, SLOT(lockPosition(Coordinate)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(unlockPositionSignal()), skeletonizer, SLOT(unlockPosition()));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(updatePositionSignal(int)), this, SLOT(updatePosition(int)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(deleteActiveNodeSignal()), skeletonizer, SLOT(delActiveNode()));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));

    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(nextCommentSignal(char*)), skeletonizer, SLOT(nextComment(char*)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(previousCommentSignal(char*)), skeletonizer, SLOT(previousComment(char*)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(popBranchNodeSignal(int,int)), skeletonizer, SLOT(popBranchNode(int,int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)), skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));

    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(delActiveTreeSignal()), skeletonizer, SLOT(delActiveTree()));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->zoomAndMultiresWidget, SIGNAL(refreshSignal()), vp, SLOT(updateGL()));

    connect(window, SIGNAL(clearSkeletonSignal(int,int)), skeletonizer, SLOT(clearSkeleton(int,int)));
    connect(window, SIGNAL(updateSkeletonFileNameSignal(int,int,char*)), skeletonizer, SLOT(updateSkeletonFileName(int,int,char*)));

    connect(vp->delegate, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    connect(vp2->delegate, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    connect(vp3->delegate, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));
    connect(vp4->delegate, SIGNAL(saveSkeletonSignal()), window, SLOT(saveSlot()));

    connect(vp->delegate, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*,int)), skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*,int)));
    connect(vp2->delegate, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*,int)), skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*,int)));
    connect(vp3->delegate, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*,int)), skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*,int)));
    connect(vp4->delegate, SIGNAL(delSegmentSignal(int,int,int,segmentListElement*,int)), skeletonizer, SLOT(delSegment(int,int,int,segmentListElement*,int)));

    connect(vp->delegate, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)), skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));
    connect(vp2->delegate, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)), skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));
    connect(vp3->delegate, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)), skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));
    connect(vp4->delegate, SIGNAL(editNodeSignal(int,int,nodeListElement*,float,int,int,int,int)), skeletonizer, SLOT(editNode(int,int,nodeListElement*,float,int,int,int,int)));

    connect(vp->delegate, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(vp2->delegate, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(vp3->delegate, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(vp4->delegate, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));

    connect(vp->delegate, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));
    connect(vp2->delegate, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));
    connect(vp3->delegate, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));
    connect(vp4->delegate, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int,int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int,int)));

    connect(vp->delegate, SIGNAL(jumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));
    connect(vp2->delegate, SIGNAL(jumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));
    connect(vp3->delegate, SIGNAL(jumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));
    connect(vp4->delegate, SIGNAL(jumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));

    connect(vp->delegate, SIGNAL(popBranchNodeSignal(int,int)), skeletonizer, SLOT(popBranchNode(int,int)));
    connect(vp2->delegate, SIGNAL(popBranchNodeSignal(int,int)), skeletonizer, SLOT(popBranchNode(int,int)));
    connect(vp3->delegate, SIGNAL(popBranchNodeSignal(int,int)), skeletonizer, SLOT(popBranchNode(int,int)));
    connect(vp4->delegate, SIGNAL(popBranchNodeSignal(int,int)), skeletonizer, SLOT(popBranchNode(int,int)));

    connect(vp->delegate, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)), skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(vp2->delegate, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)), skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(vp3->delegate, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)), skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));
    connect(vp4->delegate, SIGNAL(pushBranchNodeSignal(int,int,int,nodeListElement*,int,int)), skeletonizer, SLOT(pushBranchNode(int,int,int,nodeListElement*,int,int)));

    sendLoadSignal(state->viewerState->currentPosition.x,
                   state->viewerState->currentPosition.y,
                   state->viewerState->currentPosition.z,
                   NO_MAG_CHANGE);

    connect(skeletonizer, SIGNAL(updateToolsSignal()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));
    connect(skeletonizer, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    //connect(skeletonizer, SIGNAL(saveSkeletonSignal(int)), window, SLOT(saveSlot()));
    connect(skeletonizer, SIGNAL(userMoveSignal(int,int,int,int)), this, SLOT(userMove(int,int,int,int)));

    connect(vp->delegate, SIGNAL(findNodeInRadiusSignal(Coordinate)), skeletonizer, SLOT(findNodeInRadius(Coordinate)));
    connect(vp2->delegate, SIGNAL(findNodeInRadiusSignal(Coordinate)), skeletonizer, SLOT(findNodeInRadius(Coordinate)));
    connect(vp3->delegate, SIGNAL(findNodeInRadiusSignal(Coordinate)), skeletonizer, SLOT(findNodeInRadius(Coordinate)));
    connect(vp4->delegate, SIGNAL(findNodeInRadiusSignal(Coordinate)), skeletonizer, SLOT(findNodeInRadius(Coordinate)));

    connect(vp->delegate, SIGNAL(findSegmentByNodeIDSignal(int,int)), skeletonizer, SLOT(findSegmentByNodeIDs(int,int)));
    connect(vp2->delegate, SIGNAL(findSegmentByNodeIDSignal(int,int)), skeletonizer, SLOT(findSegmentByNodeIDs(int,int)));
    connect(vp3->delegate, SIGNAL(findSegmentByNodeIDSignal(int,int)), skeletonizer, SLOT(findSegmentByNodeIDs(int,int)));
    connect(vp->delegate, SIGNAL(findSegmentByNodeIDSignal(int,int)), skeletonizer, SLOT(findSegmentByNodeIDs(int,int)));

    connect(vp->delegate, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(vp2->delegate, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(vp3->delegate, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));
    connect(vp4->delegate, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));

    connect(vp->delegate, SIGNAL(addSkeletonNodeAndLinkWithActiveSignal(Coordinate*,Byte,int)), skeletonizer, SLOT(addSkeletonNodeAndLinkWithActive(Coordinate*,Byte,int)));
    connect(vp2->delegate, SIGNAL(addSkeletonNodeAndLinkWithActiveSignal(Coordinate*,Byte,int)), skeletonizer, SLOT(addSkeletonNodeAndLinkWithActive(Coordinate*,Byte,int)));
    connect(vp3->delegate, SIGNAL(addSkeletonNodeAndLinkWithActiveSignal(Coordinate*,Byte,int)), skeletonizer, SLOT(addSkeletonNodeAndLinkWithActive(Coordinate*,Byte,int)));
    connect(vp4->delegate, SIGNAL(addSkeletonNodeAndLinkWithActiveSignal(Coordinate*,Byte,int)), skeletonizer, SLOT(addSkeletonNodeAndLinkWithActive(Coordinate*,Byte,int)));

    connect(vp->delegate, SIGNAL(addTreeListElement(int,int,int,color4F,int)), skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(vp2->delegate, SIGNAL(addTreeListElement(int,int,int,color4F,int)), skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(vp3->delegate, SIGNAL(addTreeListElement(int,int,int,color4F,int)), skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(vp4->delegate, SIGNAL(addTreeListElement(int,int,int,color4F,int)), skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));

    connect(vp->delegate, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(vp2->delegate, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(vp3->delegate, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));
    connect(vp4->delegate, SIGNAL(idleTimeSignal()), window->widgetContainer->tracingTimeWidget, SLOT(checkIdleTime()));

    /* from x to skeletonizer´s setters */
    connect(window->widgetContainer->zoomAndMultiresWidget, SIGNAL(zoomLevelSignal(float)), skeletonizer, SLOT(setZoomLevel(float)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(showIntersectionsSignal(bool)), skeletonizer, SLOT(setShowIntersections(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(treeColorAdjustmentsChangedSignal()), window, SLOT(treeColorAdjustmentsChanged()));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(loadTreeColorTableSignal(QString,float*,int)), this, SLOT(loadTreeColorTable(QString,float*,int)));
    connect(window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SIGNAL(loadDataSetColortableSignal(QString,GLuint*,int)), this, SLOT(loadDatasetColorTable(QString,GLuint*,int)));

    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget, SIGNAL(showXYPlaneSignal(bool)), skeletonizer, SLOT(setShowXyPlane(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget, SIGNAL(rotateAroundActiveNodeSignal(bool)), skeletonizer, SLOT(setRotateAroundActiveNode(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(overrideNodeRadiusSignal(bool)), skeletonizer, SLOT(setOverrideNodeRadius(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(segRadiusToNodeRadiusSignal(float)), skeletonizer, SLOT(setSegRadiusToNodeRadius(float)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(skeletonChangedSignal(bool)), skeletonizer, SLOT(setSkeletonChanged(bool)));
    connect(window->widgetContainer->viewportSettingsWidget->generalTabWidget, SIGNAL(showNodeID(bool)), skeletonizer, SLOT(setShowNodeIDs(bool)));

    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(setActiveTreeSignal(int)), skeletonizer, SLOT(setActiveTreeByID(int)));
    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(findTreeByTreeIDSignal(int)), skeletonizer, SLOT(findTreeByTreeID(int)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(setActiveTreeSignal(int)), skeletonizer, SLOT(setActiveTreeByID(int)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(findTreeByTreeIDSignal(int)), skeletonizer, SLOT(findTreeByTreeID(int)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(restoreDefaultTreeColorSignal()), skeletonizer, SLOT(restoreDefaultTreeColor()));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(splitConnectedComponent(int,int, int)), skeletonizer, SLOT(splitConnectedComponent(int,int,int)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(addTreeListElement(int,int,int,color4F, int)), skeletonizer, SLOT(addTreeListElement(int,int,int,color4F,int)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(addTreeComment(int,int,char*)), skeletonizer, SLOT(addTreeComment(int,int,char*)));
    connect(window->widgetContainer->toolsWidget->toolsTreesTabWidget, SIGNAL(mergeTrees(int,int,int,int)), skeletonizer, SLOT(mergeTrees(int,int,int,int)));

    connect(window->widgetContainer->toolsWidget->toolsQuickTabWidget, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(addCommentSignal(int,const char*,nodeListElement*,int,int)), skeletonizer, SLOT(addComment(int,const char*,nodeListElement*,int,int)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(editCommentSignal(int,commentListElement*,int,char*,nodeListElement*,int, int)), skeletonizer, SLOT(editComment(int,commentListElement*,int,char*,nodeListElement*,int, int)));
    connect(window->widgetContainer->toolsWidget->toolsNodesTabWidget, SIGNAL(addSegmentSignal(int,int,int,int)), skeletonizer, SLOT(addSegment(int,int,int,int)));

    connect(vp->delegate, SIGNAL(updateTools()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));
    connect(vp2->delegate, SIGNAL(updateTools()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));
    connect(vp3->delegate, SIGNAL(updateTools()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));
    connect(vp4->delegate, SIGNAL(updateTools()), window->widgetContainer->toolsWidget, SLOT(updateDisplayedTree()));

    connect(vp->delegate, SIGNAL(updateSlicePlaneWidgetSignal()), window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SLOT(updateIntersection()));
    connect(vp2->delegate, SIGNAL(updateSlicePlaneWidgetSignal()), window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SLOT(updateIntersection()));
    connect(vp3->delegate, SIGNAL(updateSlicePlaneWidgetSignal()), window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SLOT(updateIntersection()));
    connect(vp4->delegate, SIGNAL(updateSlicePlaneWidgetSignal()), window->widgetContainer->viewportSettingsWidget->slicePlaneViewportWidget, SLOT(updateIntersection()));

    connect(vp->delegate, SIGNAL(moveToNextTreeSignal()), skeletonizer, SLOT(moveToNextTree()));
    connect(vp2->delegate, SIGNAL(moveToNextTreeSignal()), skeletonizer, SLOT(moveToNextTree()));
    connect(vp3->delegate, SIGNAL(moveToNextTreeSignal()), skeletonizer, SLOT(moveToNextTree()));
    connect(vp4->delegate, SIGNAL(moveToNextTreeSignal()), skeletonizer, SLOT(moveToNextTree()));

    connect(vp->delegate, SIGNAL(moveToPrevTreeSignal()), skeletonizer, SLOT(moveToPrevTree()));
    connect(vp2->delegate, SIGNAL(moveToPrevTreeSignal()), skeletonizer, SLOT(moveToPrevTree()));
    connect(vp3->delegate, SIGNAL(moveToPrevTreeSignal()), skeletonizer, SLOT(moveToPrevTree()));
    connect(vp4->delegate, SIGNAL(moveToPrevTreeSignal()), skeletonizer, SLOT(moveToPrevTree()));

    connect(vp->delegate, SIGNAL(moveToNextNodeSignal()), skeletonizer, SLOT(moveToNextNode()));
    connect(vp2->delegate, SIGNAL(moveToNextNodeSignal()), skeletonizer, SLOT(moveToNextNode()));
    connect(vp3->delegate, SIGNAL(moveToNextNodeSignal()), skeletonizer, SLOT(moveToNextNode()));
    connect(vp4->delegate, SIGNAL(moveToNextNodeSignal()), skeletonizer, SLOT(moveToNextNode()));

    connect(vp->delegate, SIGNAL(moveToPrevNodeSignal()), skeletonizer, SLOT(moveToPrevNode()));
    connect(vp2->delegate, SIGNAL(moveToPrevNodeSignal()), skeletonizer, SLOT(moveToPrevNode()));
    connect(vp3->delegate, SIGNAL(moveToPrevNodeSignal()), skeletonizer, SLOT(moveToPrevNode()));
    connect(vp4->delegate, SIGNAL(moveToPrevNodeSignal()), skeletonizer, SLOT(moveToPrevNode()));

    connect(window->widgetContainer->viewportSettingsWidget->skeletonViewportWidget, SIGNAL(updateViewerStateSignal()), this, SLOT(updateViewerState()));
    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(setActiveNodeSignal(int,nodeListElement*,int)), skeletonizer, SLOT(setActiveNode(int,nodeListElement*,int)));
    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(setJumpToActiveNodeSignal()), skeletonizer, SLOT(jumpToActiveNode()));
    connect(window->widgetContainer->commentsWidget->nodeCommentsTab, SIGNAL(findNodeByNodeIDSignal(int)), skeletonizer, SLOT(findNodeByNodeID(int)));

    connect(vp->delegate, SIGNAL(undoSignal()), skeletonizer, SLOT(undo()));
    connect(vp2->delegate, SIGNAL(undoSignal()), skeletonizer, SLOT(undo()));
    connect(vp3->delegate, SIGNAL(undoSignal()), skeletonizer, SLOT(undo()));
    connect(vp4->delegate, SIGNAL(undoSignal()), skeletonizer, SLOT(undo()));

    connect(window->widgetContainer->datasetPropertyWidget, SIGNAL(clearSkeletonSignal()), window, SLOT(clearSkeletonSlot()));

}

bool Viewer::getDirectionalVectors(float alpha, float beta, floatCoordinate *v1, floatCoordinate *v2, floatCoordinate *v3) {

        //alpha Winkel zur x-Achse
        //beta Winkel zur xy-Ebene
        alpha = alpha * PI/180;
        beta = beta * PI/180;

        SET_COORDINATE((*v1), (cos(alpha)*cos(beta)), (sin(alpha)*cos(beta)), (-sin(beta)));
        SET_COORDINATE((*v2), (-sin(alpha)), (cos(alpha)), (0));
        SET_COORDINATE((*v3), (cos(alpha)*sin(beta)), (sin(alpha)*sin(beta)), (cos(beta)));

        return true;
}

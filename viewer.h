#ifndef VIEWER_H
#define VIEWER_H

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

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
#include "knossos-global.h"

/**
 *
 *  This file contains functions that are called by the thread (entry point: start()) managing the GUI,
 *  all openGL rendering operations and
 *  all skeletonization operations commanded directly by the user over the GUI. The files gui.c, renderer.c and
 *  skeletonizer.c contain functions mainly used by the corresponding "subsystems". viewer.c contains the main
 *  event loop and routines that handle (extract slices, pack into openGL textures,...) the data coming
 *  from the loader thread.
 */

class Skeletonizer;
class Renderer;
class Viewport;
class MainWindow;
class Viewer : public QThread
{
    Q_OBJECT
public:
    explicit Viewer(QObject *parent = 0);
    Skeletonizer *skeletonizer;
    Renderer *renderer;
    MainWindow *window;

    Viewport *vp, *vp2, *vp3, *vp4;
    //from knossos-global.h

    static bool loadTreeColorTable(const char *path, float *table, int32_t type);
    //Initializes the window with the parameter given in viewerState
    static bool createScreen();
    //Transfers all (orthogonal viewports) textures completly from ram (*viewerState->vpConfigs[i].texture.data) to video memory
    //Calling makes only sense after full initialization of the SDL / OGL screen
    static bool initializeTextures(); // it now part of the initGL function of the viewport

    static bool updateZoomCube();
    static int32_t findVPnumByWindowCoordinate(uint32_t xScreen, uint32_t yScreen);

    static bool loadDatasetColorTable(const char *path, GLuint *table, int32_t type);
    bool sendLoadSignal(uint32_t x, uint32_t y, uint32_t z, int32_t magChanged);

    void run();

signals:
    void loadSignal();
    void finished();
    void updateGLSignal();
    void updateGLSignal2();
    void updateGLSignal3();
    void updateGLSignal4();

protected:
    bool sliceExtract_standard(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_adjust(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
    bool ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
public slots:
    bool changeDatasetMag(uint32_t upOrDownFlag);
    bool userMove(int32_t x, int32_t y, int32_t z, int32_t serverMovement); /* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
    bool updatePosition(int32_t serverMovement);
    bool recalcTextureOffsets();
    bool calcDisplayedEdgeLength();
    static bool refreshViewports();
    static bool updateViewerState();

protected:
    bool calcLeftUpperTexAbsPx();
    bool initViewer();

};

#endif // VIEWER_H

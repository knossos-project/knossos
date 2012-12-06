#ifndef VIEWER_H
#define VIEWER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QCursor>
extern "C" {
#include "knossos-global.h"
}
#include "knossos.h"
#include "gui.h"

class Viewer : public QObject
{
    Q_OBJECT
public:
    explicit Viewer(QObject *parent = 0);

    //from knossos-global.h
    static bool loadDatasetColorTable(const char *path, GLuint *table, int32_t type);
    static bool loadTreeColorTable(const char *path, float *table, int32_t type);
    static bool updatePosition(int32_t serverMovement);
    static bool calcDisplayedEdgeLength();

    /* upOrDownFlag can take the values: MAG_DOWN, MAG_UP */
    static bool changeDatasetMag(uint32_t upOrDownFlag);

    //Initializes the window with the parameter given in viewerState
    static bool createScreen();

    //Transfers all (orthogonal viewports) textures completly from ram (*viewerState->viewPorts[i].texture.data) to video memory
    //Calling makes only sense after full initialization of the SDL / OGL screen
    static bool initializeTextures();

    //Frees allocated memory
    static bool cleanUpViewer(struct viewerState *viewerState);

    static bool updateViewerState();
    static bool updateZoomCube();
    static bool userMove(int32_t x, int32_t y, int32_t z, int32_t serverMovement);
    static int32_t findVPnumByWindowCoordinate(uint32_t xScreen, uint32_t yScreen);
    static bool recalcTextureOffsets();
    static bool refreshViewports();

signals:
    
public slots:

public Q_SLOTS:
    //Entry point for viewer thread, general viewer coordination, "main loop"
    void start();

Q_SIGNALS:
    void finished();
    
};

#endif // VIEWER_H

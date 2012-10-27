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

class Viewer : public QThread
{
    Q_OBJECT
public:
    explicit Viewer(QObject *parent = 0);
    // Methods of viewer.c
    static vpList *vpListNew();
    static int32_t vpListAddElement(vpList *vpList, viewPort *viewPort, vpBacklog *backlog);
    static vpList *vpListGenerate(viewerState *viewerState);
    static int32_t vpListDelElement(vpList *list, vpListElement *element);
    static bool vpListDel(vpList *list);
    static vpBacklog *backlogNew();
    static int32_t backlogDelElement(vpBacklog *backlog, vpBacklogElement *element);
    static int32_t backlogAddElement(vpBacklog *backlog, Coordinate datacube, uint32_t dcOffset, Byte *slice, uint32_t x_px, uint32_t y_px, uint32_t cubeType);
    static bool backlogDel(vpBacklog *backlog);
    static bool vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState);
    static bool ocSliceExtract(Byte *datacube,
                                   Byte *slice,
                                   size_t dcOffset,
                                   struct viewPort *viewPort);
    static bool dcSliceExtract(Byte *datacube,
                                   Byte *slice,
                                   size_t dcOffset,
                                   viewPort *viewPort);
    static bool sliceExtract_standard(Byte *datacube,
                                          Byte *slice,
                                          viewPort *viewPort);
    static bool sliceExtract_adjust(Byte *datacube,
                                        Byte *slice,
                                        viewPort *viewPort);
    static bool vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState);
    static bool downsampleVPTexture(viewPort *viewPort);
    static bool upsampleVPTexture(viewPort *viewPort);

    //Calculates the upper left pixel of the texture of an orthogonal slice, dependent on viewerState->currentPosition
    static bool calcLeftUpperTexAbsPx();



    //Initializes the viewer, is called only once after the viewing thread started
    static int32_t initViewer();

    static int32_t texIndex(uint32_t x,
                            uint32_t y,
                            uint32_t colorMultiplicationFactor,
                            viewPortTexture *texture);
    static QCursor *GenCursor(char *xpm[], int xHot, int yHot);

    // Methods from knossos-global.h

signals:
    
public slots:
    
};

#endif // VIEWER_H

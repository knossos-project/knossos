#ifndef TEXTURIZER_H
#define TEXTURIZER_H

#include <QObject>
#include <slicer.h>
#include "knossos-global.h"

class Texturizer : public QObject
{
    Q_OBJECT
public:
    explicit Texturizer(QObject *parent = 0);
    bool vpGenerateTexture(vpListElement *currentVp, viewerState *viewerState);
    bool vpHandleBacklog(vpListElement *currentVp, viewerState *viewerState);
    int texIndex(uint x, uint y, uint colorMultiplicationFactor, viewportTexture *texture);
protected:
    Slicer *slicer;
public slots:
    
};

#endif // TEXTURIZER_H

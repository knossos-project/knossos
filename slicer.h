#ifndef SLICER_H
#define SLICER_H

#include <QObject>
#include "knossos-global.h"

class Slicer : public QObject
{
    Q_OBJECT
public:
    explicit Slicer(QObject *parent = 0);
    bool sliceExtract_standard(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool sliceExtract_adjust(Byte *datacube, Byte *slice, vpConfig *vpConfig);
    bool dcSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
    bool ocSliceExtract(Byte *datacube, Byte *slice, size_t dcOffset, vpConfig *vpConfig);
signals:
    
public slots:
    
};

#endif // SLICER_H

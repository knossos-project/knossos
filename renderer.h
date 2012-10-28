#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include "knossos-global.h"

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QObject *parent = 0);

    bool drawGUI();
    bool renderOrthogonalVP(uint32_t currentVP);
    bool renderSkeletonVP(uint32_t currentVP);
    uint32_t retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width);
    //Some math helper functions
    float radToDeg(float rad);
    float degToRad(float deg);
    float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
    floatCoordinate *crossProduct(floatCoordinate *v1, floatCoordinate *v2);
    float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);
    float euclidicNorm(floatCoordinate *v);
    bool normalizeVector(floatCoordinate *v);
    int32_t roundFloat(float number);
    int32_t sgn(float number);

    bool initRenderer();
    bool splashScreen();
    
signals:
    
public slots:
    
};

#endif // RENDERER_H

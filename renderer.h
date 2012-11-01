#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include "knossos-global.h"

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QObject *parent = 0);

    static bool drawGUI();
    static bool renderOrthogonalVP(uint32_t currentVP);
    static bool renderSkeletonVP(uint32_t currentVP);
    static uint32_t retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width);
    //Some math helper functions
    static float radToDeg(float rad);
    static float degToRad(float deg);
    static float scalarProduct(floatCoordinate *v1, floatCoordinate *v2);
    static floatCoordinate *crossProduct(floatCoordinate *v1, floatCoordinate *v2);
    static float vectorAngle(floatCoordinate *v1, floatCoordinate *v2);
    static float euclidicNorm(floatCoordinate *v);
    static bool normalizeVector(floatCoordinate *v);
    static int32_t roundFloat(float number);
    static int32_t sgn(float number);

    static bool initRenderer();
    static bool splashScreen();
    
signals:
    
public slots:
    
};

#endif // RENDERER_H

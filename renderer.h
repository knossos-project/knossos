#ifndef RENDERER_H
#define RENDERER_H

#define CURRENT_MAG_COORDINATES     0
#define ORIGINAL_MAG_COORDINATES    1

#define AUTOTRACING_NORMAL  0
#define AUTOTRACING_VIEWPORT    1
#define AUTOTRACING_TRACING 2
#define AUTOTRACING_MIRROR  3

#define ROTATIONSTATERESET 0
#define ROTATIONSTATEXY 1
#define ROTATIONSTATEXZ 3
#define ROTATIONSTATEYZ 2

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

    static bool setRotationState(uint32_t setTo);
    static bool rotateSkeletonViewport();
    static bool updateRotationStateMatrix(float M1[16], float M2[16]);

signals:
    
public slots:
    
};

#endif // RENDERER_H

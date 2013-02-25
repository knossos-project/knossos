#ifndef RENDERER_H
#define RENDERER_H

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

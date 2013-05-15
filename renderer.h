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

#include <QObject>
#include "knossos-global.h"

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QObject *parent = 0);    

    static bool setRotationState(uint32_t setTo);
    static bool rotateSkeletonViewport();
    static bool updateRotationStateMatrix(float M1[16], float M2[16]);
    static uint32_t renderViewportBorders(uint32_t currentVP);

    static uint32_t renderSegPlaneIntersection(struct segmentListElement *segment);
    static uint32_t renderText(Coordinate *pos, char *string);
    static uint32_t renderSphere(Coordinate *pos, float radius, color4F color, uint32_t viewportType);
    static uint32_t renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color, uint32_t viewportType);
    // void renderWholeSkeleton(uint32_t viewportType);
    static void renderSkeleton(uint32_t viewportType);
    // void renderSuperCubeSkeleton(uint32_t viewportType);
    // void renderActiveTreeSkeleton(uint32_t viewportType);
    //bool updateDisplayListsSkeleton();
    static bool doubleMeshCapacity(mesh *toDouble);
    bool initMesh(mesh *meshToInit, uint32_t initialSize);
    static bool sphereInFrustum(floatCoordinate pos, float radius, uint32_t viewportType);
    static bool updateFrustumClippingPlanes(uint32_t viewportType);
signals:
    
public slots:
    static uint32_t retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width);
    static bool renderOrthogonalVP(uint32_t currentVP);
    static bool renderSkeletonVP(uint32_t currentVP);
    bool drawGUI();
};

#endif // RENDERER_H

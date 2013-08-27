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

class Viewport;
class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QObject *parent = 0);    
    Viewport *ref, *ref2, *ref3, *ref4;
    QFont font;

protected:
    bool setRotationState(uint setTo);
    bool rotateSkeletonViewport();
    bool updateRotationStateMatrix(float M1[16], float M2[16]);
    uint renderViewportBorders(uint currentVP);

    uint renderSegPlaneIntersection(struct segmentListElement *segment);
    uint renderText(Coordinate *pos,const char *string, uint viewportType);
    uint renderSphere(Coordinate *pos, float radius, color4F color, uint viewportType);
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color, uint viewportType);
    void renderSkeleton(uint viewportType);
    bool doubleMeshCapacity(mesh *toDouble);
    bool initMesh(mesh *meshToInit, uint initialSize);
    bool sphereInFrustum(floatCoordinate pos, float radius, uint viewportType);
    bool updateFrustumClippingPlanes(uint viewportType);
signals:
    
public slots:
    uint retrieveVisibleObjectBeneathSquare(uint currentVP, uint x, uint y, uint width);
    bool renderOrthogonalVP(uint currentVP);
    bool renderSkeletonVP(uint currentVP);

};

#endif // RENDERER_H

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
#include <QList>

class Viewport;
class Renderer : public QObject {
    Q_OBJECT
    /* The first 50 entries of the openGL namespace are reserved
    for static objects (like slice plane quads...) */
    const uint GLNAME_NODEID_OFFSET = 50;//glnames for node ids start at this value
    void renderArbitrarySlicePane(const vpConfig &);
public:
    explicit Renderer(QObject *parent = 0);
    Viewport *refVPXY, *refVPXZ, *refVPYZ, *refVPSkel;
    static bool resizeMeshCapacity(Mesh *toResize, uint n);
    static bool doubleMeshCapacity(Mesh *toDouble);
    static bool initMesh(Mesh *MeshToInit, uint initialSize);
protected:
    bool setRotationState(uint setTo);
    bool rotateSkeletonViewport();
    bool updateRotationStateMatrix(float M1[16], float M2[16]);
    uint renderViewportBorders(uint currentVP);

    uint renderSegPlaneIntersection(struct segmentListElement *segment);
    void renderText(const Coordinate &pos, const QString &str);
    uint renderSphere(Coordinate *pos, float radius, Color4F color, uint currentVP, uint viewportType);
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, Color4F color, uint currentVP, uint viewportType);
    void renderSkeleton(uint currentVP,uint viewportType);    
    bool sphereInFrustum(FloatCoordinate pos, float radius, uint viewportType);
    bool updateFrustumClippingPlanes(uint viewportType);
    
public slots:
    uint retrieveVisibleObjectBeneathSquare(uint currentVP, uint x, uint y, uint width);
    void retrieveAllObjectsBeneathSquare(uint currentVP, uint x, uint y, uint width, uint height);
    bool renderOrthogonalVP(uint currentVP);
    bool renderSkeletonVP(uint currentVP);
    void renderUserGeometry();

};

#endif // RENDERER_H

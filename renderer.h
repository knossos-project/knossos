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
#include "scriptengine/decorators/meshdecorator.h"
#include <QList>

class Viewport;

class ColorPickBuffer : public QObject {
    uint vpId;

    Coordinate position;

public:
    uint size;
    GLubyte *buffer;

    ColorPickBuffer() : vpId(0), size(0), buffer(nullptr) {
        SET_COORDINATE(position, -1, -1, -1);
    }

    void update(uint currVp, uint currSize, Coordinate currPos) {
        vpId = currVp;
        size = currSize;
        SET_COORDINATE(position, currPos.x, currPos.y, currPos.z);
        free(buffer);
        buffer = (GLubyte*)calloc(3, size * size);
    }

    bool upToDate(uint currVp, uint currSize, Coordinate currPos) {
        return currVp == vpId && size == currSize && COMPARE_COORDINATE(currPos, position);
    }

    std::tuple<uint8_t, uint8_t, uint8_t> getColor(uint x, uint y) {
        if(x > size || y > size) {
            return std::make_tuple(0, 0, 0);
        }
        return std::make_tuple(*(buffer + 0 + ((size - y)*size*3) + (x*3)),
                               *(buffer + 1 + ((size - y)*size*3) + (x*3)),
                               *(buffer + 2 + ((size - y)*size*3) + (x*3)));
    }

    ~ColorPickBuffer() {
        free(buffer);
    }
};

class Renderer : public QObject {
    Q_OBJECT
    friend class MeshDecorator;
    /* The first 50 entries of the openGL namespace are reserved
    for static objects (like slice plane quads...) */
    const uint GLNAME_NODEID_OFFSET = 50;//glnames for node ids start at this value
    void renderArbitrarySlicePane(const vpConfig &);
    ColorPickBuffer pickBuffer;
public:
    explicit Renderer(QObject *parent = 0);
    Viewport *refVPXY, *refVPXZ, *refVPYZ, *refVPSkel;
    void renderMergeCursor(uint viewportType, const int x, const int y);
protected:
    bool setRotationState(uint setTo);
    bool rotateSkeletonViewport();
    bool updateRotationStateMatrix(float M1[16], float M2[16]);
    uint renderViewportBorders(uint currentVP);

    uint renderSegPlaneIntersection(struct segmentListElement *segment);
    void renderText(const Coordinate &pos, const QString &str);
    uint renderSphere(Coordinate *pos, float radius, color4F color, uint currentVP, uint viewportType);
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color, uint currentVP, uint viewportType);
    void renderSkeleton(uint currentVP,uint viewportType);
    void renderMergeLine(uint viewportType);
    bool resizemeshCapacity(mesh *toResize, uint n);
    bool doubleMeshCapacity(mesh *toDouble);
    bool initMesh(mesh *meshToInit, uint initialSize);
    bool sphereInFrustum(floatCoordinate pos, float radius, uint viewportType);
    bool updateFrustumClippingPlanes(uint viewportType);

public slots:
    uint retrieveVisibleObjectBeneathSquare(uint currentVP, uint x, uint y, uint width);
    std::vector<nodeListElement *> retrieveAllObjectsBeneathSquare(uint currentVP, uint x, uint y, uint width, uint height);
    std::tuple<uint8_t, uint8_t, uint8_t> retrieveUniqueColorFromPixel(uint currentVP, const uint x, const uint y);
    bool renderOrthogonalVP(uint currentVP);
    bool renderSkeletonVP(uint currentVP);
};

#endif // RENDERER_H

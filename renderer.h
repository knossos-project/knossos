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
#include "scriptengine/decorators/meshdecorator.h"
#include "widgets/viewport.h"

#include <QList>
#include <QObject>

#include <boost/multi_array.hpp>

// skeleton vp orientation
#define SKELVP_XY_VIEW 0
#define SKELVP_XZ_VIEW 1
#define SKELVP_YZ_VIEW 2
#define SKELVP_R90 3
#define SKELVP_R180 4
#define SKELVP_RESET 5

constexpr int defaultFonsSize = 10;

class Viewport;
class segmentListElement;

struct RenderOptions {
    RenderOptions(const bool drawBoundaryBox = true, const bool drawCrosshairs = true, const bool drawOverlay = true, const bool drawSkeleton = true,
                  const bool drawViewportPlanes = true, const bool highlightActiveNode = true, const bool highlightSelection = true)
        : drawBoundaryBox(drawBoundaryBox), drawCrosshairs(drawCrosshairs), drawOverlay(drawOverlay),drawSkeleton(drawSkeleton),
          drawViewportPlanes(drawViewportPlanes), highlightActiveNode(highlightActiveNode), highlightSelection(highlightSelection) {}
    bool drawBoundaryBox;
    bool drawCrosshairs;
    bool drawOverlay;
    bool drawSkeleton;
    bool drawViewportPlanes;
    bool highlightActiveNode;
    bool highlightSelection;
};

class Renderer : public QObject {
    Q_OBJECT
    friend class MeshDecorator;
    /* The first 50 entries of the openGL namespace are reserved
    for static objects (like slice plane quads...) */
    const uint GLNAME_NODEID_OFFSET = 50;//glnames for node ids start at this value
    void renderArbitrarySlicePane(const vpConfig &);
public:
    explicit Renderer(QObject *parent = 0);
    Viewport *refVPXY, *refVPXZ, *refVPYZ, *refVPSkel;
    void renderBrush(uint viewportType, Coordinate coord);
    void setFrontFacePerspective(uint currentVP);
    void renderViewportFrontFace(uint currentVP);
    void renderSizeLabel(uint currentVP, const int fontSize = defaultFonsSize);
    void renderScaleBar(uint currentVP, const int thickness, const int fontSize = defaultFonsSize);
protected:
    bool setRotationState(uint setTo);
    bool rotateSkeletonViewport();
    bool updateRotationStateMatrix(float M1[16], float M2[16]);

    uint renderSegPlaneIntersection(segmentListElement *segment);
    void renderText(const Coordinate &pos, const QString &str, const int fontSize = defaultFonsSize, const bool centered = false);
    uint renderSphere(Coordinate *pos, float radius, color4F color, uint currentVP, uint viewportType);
    uint renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color, uint currentVP, uint viewportType);
    void renderSkeleton(uint currentVP,uint viewportType, const RenderOptions & options = RenderOptions());
    bool resizemeshCapacity(mesh *toResize, uint n);
    bool doubleMeshCapacity(mesh *toDouble);
    bool initMesh(mesh *meshToInit, uint initialSize);
    bool sphereInFrustum(floatCoordinate pos, float radius, uint viewportType);
    bool updateFrustumClippingPlanes(uint viewportType);

public slots:
    uint retrieveVisibleObjectBeneathSquare(uint currentVP, uint x, uint y, uint width);
    std::vector<nodeListElement *> retrieveAllObjectsBeneathSquare(uint currentVP, uint centerX, uint centerY, uint width, uint height);
    bool renderOrthogonalVP(uint currentVP, const RenderOptions & options = RenderOptions());
    bool renderSkeletonVP(const RenderOptions & options = RenderOptions());
};

#endif // RENDERER_H

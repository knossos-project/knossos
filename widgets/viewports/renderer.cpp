/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "annotation/annotation.h"
#include "dataset.h"
#include "functions.h"
#include "widgets/viewports/viewportarb.h"
#include "widgets/viewports/viewportbase.h"
#include "widgets/viewports/viewportortho.h"
#include "widgets/viewports/viewport3d.h"

#include "profiler.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "stateInfo.h"
#include "viewer.h"

#include <QMatrix4x4>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QOpenGLTimeMonitor>
#include <QPainter>
#include <QQuaternion>
#include <QVector3D>

#ifdef Q_OS_MAC
    #include <glu.h>
#else
    #include <GL/glu.h>
#endif

#include <boost/math/constants/constants.hpp>

#include <array>
#include <cmath>

enum GLNames {
    None,
    Scalebar,
    NodeOffset
};

auto uniformPointDiameter(const float pixelPerNanometer) {
    return pixelPerNanometer * Dataset::current().scales[0].x * 2 * (state->viewerState->overrideNodeRadiusBool ? state->viewerState->overrideNodeRadiusVal : 1.5f);
}

auto smallestVisibleNodeSize() {
    return state->viewerState->sampleBuffers == 0 ? 1 : 1.0f/state->viewerState->sampleBuffers;
}

auto pointSize(const float pixelPerNanometer) {
    return std::max(smallestVisibleNodeSize(), uniformPointDiameter(pixelPerNanometer));
}

auto lineSize(const float nanometerPerPixel) {
    return std::max(smallestVisibleNodeSize(), state->viewerState->segRadiusToNodeRadius * uniformPointDiameter(nanometerPerPixel));
}

QColor getPickingColor(const nodeListElement & node, const RenderOptions::SelectionPass &selectionPass) {
    QColor color;

    const auto name = GLNames::NodeOffset + node.nodeID;

    int shift{0};
    if (selectionPass == RenderOptions::SelectionPass::NodeID24_48Bits) {
        shift = 24;
    } else if (selectionPass == RenderOptions::SelectionPass::NodeID48_64Bits) {
        shift = 48;
    }
    const auto bits = static_cast<GLuint>(name >> shift);// extract 24 first, middle or 16 last bits of interest
    color.setRed(static_cast<std::uint8_t>(bits));
    color.setGreen(static_cast<std::uint8_t>(bits >> 8));
    color.setBlue(static_cast<std::uint8_t>(bits >> 16));
    color.setAlpha(255);

    return color;
}

void ViewportBase::renderCylinder(const Coordinate & base, float baseRadius, const Coordinate & top, float topRadius, const QColor & color, const RenderOptions & options) {
    const auto isoBase = Dataset::current().scales[0].componentMul(base);
    const auto isoTop = Dataset::current().scales[0].componentMul(top);
    baseRadius *= Dataset::current().scales[0].x;
    topRadius *= Dataset::current().scales[0].x;

    if (!options.useLinesAndPoints(std::max(baseRadius, topRadius) * screenPxXPerDataPx, smallestVisibleNodeSize())) {
        glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());

        glPushMatrix();
        GLUquadricObj * gluCylObj = gluNewQuadric();
        gluQuadricNormals(gluCylObj, GLU_SMOOTH);
        gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);

        glTranslatef(isoBase.x, isoBase.y, isoBase.z);

        //Some calculations for the correct direction of the cylinder.
        const floatCoordinate cylinderDirection{0.0f, 0.0f, 1.0f};
        floatCoordinate segDirection{isoTop - isoBase};

        floatCoordinate rotationAxis{cylinderDirection.cross(segDirection)};
        const float currentAngle{radToDeg(cylinderDirection.angleRad(segDirection))};
        //we need another reference vector for 180° rotations
        if (rotationAxis == floatCoordinate{0, 0, 0}) {
            rotationAxis = {floatCoordinate(0.0f, 1.0f, 0.0f).cross(segDirection)};
        }
        //some gl implementations have problems with the params occuring for
        //segs in straight directions. we need a fix here.
        glRotatef(currentAngle, rotationAxis.x, rotationAxis.y, rotationAxis.z);

        //tdItem use screenPxXPerDataPx for proper LOD
        //the same values have to be used in rendersegplaneintersections to avoid ugly graphics
        const auto edges = std::max(baseRadius, topRadius) > 100.f ? 10 : std::max(baseRadius, topRadius) > 15.f ? 6 : 3;
        gluCylinder(gluCylObj, baseRadius, topRadius, segDirection.length(), edges, 1);

        gluDeleteQuadric(gluCylObj);
        glPopMatrix();
    }
}

void ViewportBase::renderSphere(const Coordinate & pos, float radius, const QColor & color, const RenderOptions & options) {
    const auto isoPos = Dataset::current().scales[0].componentMul(pos);
    radius *= Dataset::current().scales[0].x;

    if (!options.useLinesAndPoints(radius * screenPxXPerDataPx, smallestVisibleNodeSize())) {
        glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        glPushMatrix();
        glTranslatef(isoPos.x, isoPos.y, isoPos.z);
        auto * gluSphereObj = gluNewQuadric();
        gluQuadricNormals(gluSphereObj, GLU_SMOOTH);
        gluQuadricOrientation(gluSphereObj, GLU_OUTSIDE);

        if(radius * screenPxXPerDataPx  > 20.) {
            gluSphere(gluSphereObj, radius, 14, 14);
        }
        else if(radius * screenPxXPerDataPx  > 5.) {
            gluSphere(gluSphereObj, radius, 8, 8);
        }
        else {
            gluSphere(gluSphereObj, radius, 5, 5);
        }
        gluDeleteQuadric(gluSphereObj);
        glPopMatrix();
    }
}

void ViewportBase::renderSegment(const segmentListElement & segment, const QColor & color, const RenderOptions & options) {
    renderCylinder(segment.source.position, Skeletonizer::singleton().radius(segment.source) * state->viewerState->segRadiusToNodeRadius,
        segment.target.position, Skeletonizer::singleton().radius(segment.target) * state->viewerState->segRadiusToNodeRadius, color, options);
}

void ViewportOrtho::renderSegment(const segmentListElement & segment, const QColor & color, const RenderOptions & options) {
    ViewportBase::renderSegment(segment, color, options);
    if (state->viewerState->showIntersections) {
        renderSegPlaneIntersection(segment);
    }
}

void ViewportBase::renderNode(const nodeListElement & node, const RenderOptions & options) {
    auto color = state->viewer->getNodeColor(node);
    const float radius = Skeletonizer::singleton().radius(node);

    if (options.nodePicking) {
        color = getPickingColor(node, options.selectionPass);
    }

    renderSphere(node.position, radius, color, options);

    // halo only 10% bigger then node or
    // scale the halo size from 10% up to 100% bigger then the node size
    // otherwise, the halo won't be visible on very small node sizes (=1.0)
    // (x - x_0) * (f_1 - f_0)/(x_1 - x_0)  //Linear interpolation
    const float mp = radius > 30.0f ? 1.1 : (radius - 1.0) * (1.1 - 2.0)/(30.0 - 1.0) + 2.0;
    if (node.selected && options.highlightSelection) {// highlight selected nodes
        auto selectedNodeColor = QColor(Qt::green);
        selectedNodeColor.setAlphaF(0.5f);
        renderSphere(node.position, radius * mp, selectedNodeColor);
    }
    if (state->skeletonState->activeNode == &node && options.highlightActiveNode) {// highlight active node with a halo
        // Color gets changes in case there is a comment & conditional comment highlighting
        auto haloColor = state->viewer->getNodeColor(node);
        haloColor.setAlphaF(0.2);
        renderSphere(node.position, radius * ((mp - 1.0)/2.0 + 1.0), haloColor);
    }
}

void ViewportOrtho::renderNode(const nodeListElement & node, const RenderOptions & options) {
    ViewportBase::renderNode(node, options);
    if (1.5f <  Skeletonizer::singleton().radius(node)) { // draw node center to make large nodes visible and clickable in ortho vps
        renderSphere(node.position, 1.5, state->viewer->getNodeColor(node));
    }
    if (!options.nodePicking) {
        // Render the node description
        glColor4f(0.f, 0.f, 0.f, 1.f);
        auto nodeID = (state->viewerState->idDisplay.testFlag(IdDisplay::AllNodes) || (state->viewerState->idDisplay.testFlag(IdDisplay::ActiveNode) && state->skeletonState->activeNode == &node))? QString::number(node.nodeID) : "";
        auto comment = node.getComment();
        comment = (ViewportOrtho::showNodeComments && comment.isEmpty() == false)? QString(":%1").arg(comment) : "";
        if (nodeID.isEmpty() == false || comment.isEmpty() == false) {
            renderText(Dataset::current().scales[0].componentMul(node.position), nodeID.append(comment), options.enableTextScaling);
        }
    }
}

void Viewport3D::renderNode(const nodeListElement & node, const RenderOptions & options) {
    ViewportBase::renderNode(node, options);
    if (!options.nodePicking) {
        // Render the node description
        if (state->viewerState->idDisplay.testFlag(IdDisplay::AllNodes) || (state->viewerState->idDisplay.testFlag(IdDisplay::ActiveNode) && state->skeletonState->activeNode == &node)) {
            glColor4f(0.f, 0.f, 0.f, 1.f);
            renderText(Dataset::current().scales[0].componentMul(node.position), QString::number(node.nodeID), options.enableTextScaling);
        }
    }
}

static void backup_gl_state() {
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ALL_ATTRIB_BITS | GL_MULTISAMPLE_BIT);// http://mesa-dev.freedesktop.narkive.com/4FYSpYiY/patch-mesa-change-gl-all-attrib-bits-to-0xffffffff
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
}

static void restore_gl_state() {
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
    glPopClientAttrib();
}

void ViewportBase::renderText(const Coordinate & pos, const QString & str, const bool fontScaling, bool centered) {
    GLdouble x, y, z, model[16], projection[16];
    GLint gl_viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, &model[0]);
    glGetDoublev(GL_PROJECTION_MATRIX, &projection[0]);
    glGetIntegerv(GL_VIEWPORT, gl_viewport);
    //retrieve 2d screen position from coordinate
    backup_gl_state();
    QOpenGLPaintDevice paintDevice(gl_viewport[2], gl_viewport[3]);//create paint device from viewport size and current context
    QPainter painter(&paintDevice);
    painter.setFont(QFont(painter.font().family(), (fontScaling ? std::ceil(0.02*gl_viewport[2]) : defaultFontSize) * devicePixelRatio()));
    gluProject(pos.x, pos.y - 0.01*edgeLength, pos.z, &model[0], &projection[0], &gl_viewport[0], &x, &y, &z);
    painter.setPen(Qt::black);
    painter.drawText(centered ? x - QFontMetrics(painter.font()).horizontalAdvance(str)/2. : x, gl_viewport[3] - y, str);//inverse y coordinate, extract height from gl viewport
    painter.end();//would otherwise fiddle with the gl state in the dtor
    restore_gl_state();
}

void ViewportOrtho::renderSegPlaneIntersection(const segmentListElement & segment) {
    float p[2][3], a, currentAngle, length, radius, distSourceInter;
    floatCoordinate tempVec2, tempVec, tempVec3, segDir, intPoint;
    GLUquadricObj *gluCylObj = NULL;

    const auto isoCurPos = Dataset::current().scales[0].componentMul(state->viewerState->currentPosition);

    const auto sSp_local = Dataset::current().scales[0].componentMul(segment.source.position);
    const auto sTp_local = Dataset::current().scales[0].componentMul(segment.target.position);

    const auto sSr_local = segment.source.radius * Dataset::current().scales[0].x;
    const auto sTr_local = segment.target.radius * Dataset::current().scales[0].x;

    //n contains the normal vectors of the 3 orthogonal planes
    float n[3][3] = {{1.,0.,0.},
                    {0.,1.,0.},
                    {0.,0.,1.}};

    const auto distToCurrPos = 0.5 * (state->M / 2) * Dataset::current().cubeEdgeLength * Dataset::current().magnification * Dataset::current().scales[0].x;

    //Check if there is an intersection between the given segment and one
    //of the slice planes.
    p[0][0] = sSp_local.x - isoCurPos.x;
    p[0][1] = sSp_local.y - isoCurPos.y;
    p[0][2] = sSp_local.z - isoCurPos.z;

    p[1][0] = sTp_local.x - isoCurPos.x;
    p[1][1] = sTp_local.y - isoCurPos.y;
    p[1][2] = sTp_local.z - isoCurPos.z;


    //i represents the current orthogonal plane
    for (int i = 0; i <= 2; ++i) {
        //There is an intersection and the segment doesn't lie in the plane
        if(sgn(p[0][i])*sgn(p[1][i]) == -1) {
            //Calculate intersection point
            segDir.x = sTp_local.x - sSp_local.x;
            segDir.y = sTp_local.y - sSp_local.y;
            segDir.z = sTp_local.z - sSp_local.z;

            //a is the scaling factor for the straight line equation: g:=segDir*a+v0
            a = (n[i][0] * ((isoCurPos.x - sSp_local.x))
                    + n[i][1] * ((isoCurPos.y - sSp_local.y))
                    + n[i][2] * ((isoCurPos.z - sSp_local.z)))
                / (segDir.x*n[i][0] + segDir.y*n[i][1] + segDir.z*n[i][2]);

            tempVec3.x = segDir.x * a;
            tempVec3.y = segDir.y * a;
            tempVec3.z = segDir.z * a;

            intPoint.x = tempVec3.x + sSp_local.x;
            intPoint.y = tempVec3.y + sSp_local.y;
            intPoint.z = tempVec3.z + sSp_local.z;

            //Check wether the intersection point lies outside the current zoom cube
            if(std::abs((int)intPoint.x - isoCurPos.x) <= distToCurrPos
                && std::abs((int)intPoint.y - isoCurPos.y) <= distToCurrPos
                && std::abs((int)intPoint.z - isoCurPos.z) <= distToCurrPos) {

                //Render a cylinder to highlight the intersection
                glPushMatrix();
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);

                length = segDir.length();
                distSourceInter = tempVec3.length();

                if (sSr_local < sTr_local) {
                    radius = sTr_local + sSr_local * (1. - distSourceInter / length);
                } else if(sSr_local == sTr_local) {
                    radius = sSr_local;
                } else {
                    radius = sSr_local - (sSr_local - sTr_local) * distSourceInter / length;
                }

                segDir = {segDir.x / length, segDir.y / length, segDir.z / length};

                glTranslatef(intPoint.x, intPoint.y, intPoint.z);

                //Some calculations for the correct direction of the cylinder.
                tempVec = {0., 0., 1.};
                //temVec2 defines the rotation axis
                tempVec2 = tempVec.cross(segDir);
                currentAngle = radToDeg(tempVec.angleRad(segDir));
                //we need another reference vector for 180° rotations
                if (tempVec2 == floatCoordinate{0, 0, 0}) {
                    tempVec2 = {floatCoordinate(0.0f, 1.0f, 0.0f).cross(segDir)};
                }
                glRotatef(currentAngle, tempVec2.x, tempVec2.y, tempVec2.z);

                glColor4f(0.,0.,0.,1.);

                auto cylinderRadius = state->viewerState->overrideNodeRadiusBool ? state->viewerState->overrideNodeRadiusVal : radius;
                cylinderRadius *= state->viewerState->segRadiusToNodeRadius * 1.2;
                gluCylinder(gluCylObj, cylinderRadius, cylinderRadius, 1.5 * Dataset::current().scales[0].x, 3, 1);

                gluDeleteQuadric(gluCylObj);
                glPopMatrix();
            }

        }
    }
}

void ViewportBase::setFrontFacePerspective() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* define coordinate system for our viewport: left right bottom top near far */
    glOrtho(0, edgeLength, edgeLength, 0, 25, -25);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void ViewportBase::renderViewportFrontFace() {
    setFrontFacePerspective();
    // render node selection box
    if (state->viewerState->nodeSelectSquareData.first == static_cast<int>(viewportType)) {
        Coordinate leftUpper = state->viewerState->nodeSelectionSquare.first;
        Coordinate rightLower = state->viewerState->nodeSelectionSquare.second;

        glLineWidth(1.);
        glBegin(GL_QUADS);
        glColor4f(0, 1., 0, 0.2);
            glVertex3f(leftUpper.x, leftUpper.y, 0.f);
            glVertex3f(leftUpper.x, rightLower.y, 0.f);
            glVertex3f(rightLower.x, rightLower.y, 0.f);
            glVertex3f(rightLower.x, leftUpper.y, 0.f);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glColor4f(0, 1., 0, 1);
            glVertex3f(leftUpper.x, leftUpper.y, 0.f);
            glVertex3f(leftUpper.x, rightLower.y, 0.f);
            glVertex3f(rightLower.x, rightLower.y, 0.f);
            glVertex3f(rightLower.x, leftUpper.y, 0.f);
        glEnd();
    }
}

void ViewportOrtho::renderViewportFrontFace() {
    ViewportBase::renderViewportFrontFace();
    glColor4f(0.7f * std::abs(n.z), 0.7f * std::abs(n.y), 0.7f * std::abs(n.x), 1);
    glLineWidth(2.);
    glBegin(GL_LINES);
        glVertex3d(1, 1, -1);
        glVertex3d(edgeLength - 1, 1, -1);
        glVertex3d(edgeLength - 1, 1, -1);
        glVertex3d(edgeLength - 1, edgeLength - 1, -1);
        glVertex3d(edgeLength - 1, edgeLength - 1, -1);
        glVertex3d(1, edgeLength - 1, -1);
        glVertex3d(1, edgeLength - 1, -1);
        glVertex3d(1, 1, -1);
    glEnd();

    if (viewportType == state->viewerState->highlightVp) {
        // Draw an orange border to highlight the viewport.
        glColor4f(1., 0.3, 0., 1.);
        glBegin(GL_LINE_LOOP);
            glVertex3f(3, 3, -1);
            glVertex3f(edgeLength - 3, 3, -1);
            glVertex3f(edgeLength - 3, edgeLength - 3, -1);
            glVertex3f(3, edgeLength - 3, -1);
        glEnd();
    }

    if (state->viewerState->showScalebar) {
        renderScaleBar();
    }
}

void Viewport3D::renderViewportFrontFace() {
    ViewportBase::renderViewportFrontFace();
    glColor4f(0, 0, 0, 1.);
    glLineWidth(2.);
    glBegin(GL_LINES);
        glVertex3d(1, 1, -1);
        glVertex3d(edgeLength - 1, 1, -1);
        glVertex3d(edgeLength - 1, 1, -1);
        glVertex3d(edgeLength - 1, edgeLength - 1, -1);
        glVertex3d(edgeLength - 1, edgeLength - 1, -1);
        glVertex3d(1, edgeLength - 1, -1);
        glVertex3d(1, edgeLength - 1, -1);
        glVertex3d(1, 1, -1);
    glEnd();
    if (Segmentation::singleton().volume_render_toggle == false && state->viewerState->showScalebar) {
        renderScaleBar();
    }
}

auto getScaleBarLabelNmAndPx(double vpLenNm, int edgeLength) {
    auto scalebarLenNm = vpLenNm / 3.0;
    const auto powerOf10 = std::trunc(std::log10(scalebarLenNm));
    const auto roundStep = 5.0 * std::pow(10, powerOf10 - 1);
    scalebarLenNm = std::round(scalebarLenNm / roundStep) * roundStep;
    const auto scalebarLenPx = edgeLength * scalebarLenNm / vpLenNm;

    QString sizeLabel{"nm"};
    auto scalebarLen = scalebarLenNm;
    if (powerOf10 > 6.0) {
        sizeLabel = "cm";
        scalebarLen /= 1e7;
    } else if (powerOf10 == 6.0) {
        sizeLabel = "mm";
        scalebarLen /= 1e6;
    } else if (powerOf10 >= 3.0) {
        sizeLabel = "μm";
        scalebarLen /= 1e3;
    }
    sizeLabel = QString::number(scalebarLen) + " " + sizeLabel;

    return std::make_tuple(sizeLabel, scalebarLenNm, scalebarLenPx);
}

void ViewportBase::renderScaleBar() {
    QString sizeLabel;
    int scalebarLenPx{};
    std::tie(sizeLabel, std::ignore, scalebarLenPx) = getScaleBarLabelNmAndPx(displayedlengthInNmX, edgeLength);

    const int margin =  0.02 * edgeLength;
    const int height = 0.007 * edgeLength;
    Coordinate min(margin,  edgeLength - margin - height, -1);
    Coordinate max(min.x + scalebarLenPx, min.y + height, -1);
    glColor3f(0., 0., 0.);
    glBegin(GL_POLYGON);
    glVertex3d(min.x, min.y, min.z);
    glVertex3d(max.x, min.y, min.z);
    glVertex3d(max.x, max.y, min.z);
    glVertex3d(min.x, max.y, min.z);
    glEnd();

    renderText(Coordinate(min.x + scalebarLenPx / 2, min.y, min.z), sizeLabel, true, true);
}

void ViewportOrtho::renderViewportFast() {
    if (state->viewer->layers.empty()) {
        return;
    }

    QOpenGLTimeMonitor times;
    times.setSampleCount(3);
    times.create();
    times.recordSample();

    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const bool xy = viewportType == VIEWPORT_XY;
    const bool xz = viewportType == VIEWPORT_XZ;
    const bool zy = viewportType == VIEWPORT_ZY;
    const bool arb = viewportType == VIEWPORT_ARBITRARY;
    const float gpucubeedge = state->viewer->gpucubeedge;
    const auto fov = (state->M - 1) * Dataset::current().cubeEdgeLength / (arb ? std::sqrt(2) : 1);//remove cpu overlap
    const auto gpusupercube = fov / gpucubeedge + 1;//add gpu overlap
    floatCoordinate cpos = state->viewerState->currentPosition;
    const auto scale = Dataset::current().scale.z / Dataset::current().scale.x;
    if (arb) {
        cpos.z *= scale;
    }

    std::vector<std::array<GLfloat, 3>> triangleVertices;
    triangleVertices.reserve(6);
    triangleVertices.push_back({{0.0f, 0.0f, 0.0f}});
    triangleVertices.push_back({{gpucubeedge, 0.0f, 0.0f}});
    triangleVertices.push_back({{gpucubeedge, gpucubeedge, 0.0f}});
    triangleVertices.push_back({{0.0f, gpucubeedge, 0.0f}});
    std::vector<std::array<GLfloat, 3>> textureVertices;
    textureVertices.reserve(6);
    for (float z = 0.0f; z < (xy ? 1 : gpusupercube); ++z)
    for (float y = 0.0f; y < (xz ? 1 : gpusupercube); ++y)
    for (float x = 0.0f; x < (zy ? 1 : gpusupercube); ++x) {
        const float frame = std::fmod(xy ? cpos.z : xz ? cpos.y : cpos.x, state->viewer->gpucubeedge);
        const auto texR = (0.5f + frame) / gpucubeedge;

        if (xy) {
            textureVertices.push_back({{0.0f, 0.0f, texR}});
            textureVertices.push_back({{1.0f, 0.0f, texR}});
            textureVertices.push_back({{1.0f, 1.0f, texR}});
            textureVertices.push_back({{0.0f, 1.0f, texR}});
        } else if (xz) {
            textureVertices.push_back({{0.0f, texR, 0.0f}});
            textureVertices.push_back({{1.0f, texR, 0.0f}});
            textureVertices.push_back({{1.0f, texR, 1.0f}});
            textureVertices.push_back({{0.0f, texR, 1.0f}});
        } else if (zy) {
            textureVertices.push_back({{texR, 0.0f, 0.0f}});
            textureVertices.push_back({{texR, 0.0f, 1.0f}});
            textureVertices.push_back({{texR, 1.0f, 1.0f}});
            textureVertices.push_back({{texR, 1.0f, 0.0f}});
        }
    }

    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;
    projectionMatrix.ortho(-0.5 * width(), 0.5 * width(), -0.5 * height(), 0.5 * height(), -scale * gpucubeedge, scale * gpucubeedge);

    //z component of vp vectors specifies portion of scale to apply
    const auto zScaleIncrement = !arb ? scale - 1 : 0;
    const float hfov = texture.FOV * fov / (1 + zScaleIncrement * std::abs(v1.z));
    const float vfov = texture.FOV * fov / (1 + zScaleIncrement * std::abs(v2.z));
    viewMatrix.scale(width() / hfov, height() / vfov);
    const auto cameraPos = floatCoordinate{cpos} + n;
    viewMatrix.lookAt(cameraPos, cpos, v2);

    // raw data shader
    raw_data_shader.bind();
    int vertexLocation = raw_data_shader.attributeLocation("vertex");
    int texLocation = raw_data_shader.attributeLocation("texCoordVertex");
    raw_data_shader.enableAttributeArray(vertexLocation);
    raw_data_shader.enableAttributeArray(texLocation);
    raw_data_shader.setAttributeArray(vertexLocation, triangleVertices.data()->data(), 3);
    raw_data_shader.setAttributeArray(texLocation, textureVertices.data()->data(), 3);
    raw_data_shader.setUniformValue("view_matrix", viewMatrix);
    raw_data_shader.setUniformValue("projection_matrix", projectionMatrix);
    raw_data_shader.setUniformValue("texture", 0);

    // overlay data shader
    overlay_data_shader.bind();
    int overtexLocation = overlay_data_shader.attributeLocation("vertex");
    int otexLocation = overlay_data_shader.attributeLocation("texCoordVertex");
    overlay_data_shader.enableAttributeArray(overtexLocation);
    overlay_data_shader.enableAttributeArray(otexLocation);
    overlay_data_shader.setAttributeArray(overtexLocation, triangleVertices.data()->data(), triangleVertices.data()->size());
    overlay_data_shader.setAttributeArray(otexLocation, textureVertices.data()->data(), textureVertices.data()->size());
    overlay_data_shader.setUniformValue("view_matrix", viewMatrix);
    overlay_data_shader.setUniformValue("projection_matrix", projectionMatrix);
    overlay_data_shader.setUniformValue("indexTexture", 0);
    overlay_data_shader.setUniformValue("textureLUT", 1);
    overlay_data_shader.setUniformValue("factor", static_cast<float>(std::numeric_limits<gpu_lut_cube::gpu_index>::max()));

    glEnable(GL_TEXTURE_3D);

    for (auto & layer : state->viewer->layers) {
        if (layer.enabled && layer.opacity >= 0.0f && !(state->viewerState->showOnlyRawData && layer.isOverlayData)) {
            if (layer.isOverlayData) {
                overlay_data_shader.bind();
                overlay_data_shader.setUniformValue("textureOpacity", Segmentation::singleton().alpha / 256.0f);
            } else {
                raw_data_shader.bind();
                raw_data_shader.setUniformValue("textureOpacity", layer.opacity);
            }

            auto render = [&](auto & cube, const QMatrix4x4 modelMatrix = {}){
                if (layer.isOverlayData) {
                    auto & punned = static_cast<gpu_lut_cube&>(cube);
                    punned.cube.bind(0);
                    punned.lut.bind(1);
                    overlay_data_shader.setUniformValue("model_matrix", modelMatrix);
                    overlay_data_shader.setUniformValue("lutSize", static_cast<float>(punned.lut.width() * punned.lut.height() * punned.lut.depth()));
                } else {
                    raw_data_shader.setUniformValue("model_matrix", modelMatrix);
                    cube.cube.bind(0);
                }
                glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<int>(triangleVertices.size()));
            };
            if (!arb) {
                const float halfsc = fov * 0.5f / gpucubeedge;
                const float offsetx = cpos.x / gpucubeedge - halfsc * !zy;
                const float offsety = cpos.y / gpucubeedge - halfsc * !xz;
                const float offsetz = cpos.z / gpucubeedge - halfsc * !xy;
                const float startx = 0 * cpos.x / gpucubeedge;
                const float starty = 0 * cpos.y / gpucubeedge;
                const float startz = 0 * cpos.z / gpucubeedge;
                const float endx = startx + (zy ? 1 : gpusupercube);
                const float endy = starty + (xz ? 1 : gpusupercube);
                const float endz = startz + (xy ? 1 : gpusupercube);
                for (float z = startz; z < endz; ++z)
                for (float y = starty; y < endy; ++y)
                for (float x = startx; x < endx; ++x) {
                    const auto pos = CoordOfGPUCube(offsetx + x, offsety + y, offsetz + z);
                    auto it = layer.textures.find(pos);
                    auto & ptr = it != std::end(layer.textures) ? *it->second : *layer.bogusCube;

                    QMatrix4x4 modelMatrix;
                    modelMatrix.translate(pos.x * gpucubeedge, pos.y * gpucubeedge, pos.z * gpucubeedge);
                    modelMatrix.scale(1, 1 - 2*(zy + xy), 1 - 2*xz);// HACK still don’t know
                    modelMatrix.rotate(QQuaternion::fromAxes(v1, v2, n));

                    render(ptr, modelMatrix);
                }
            } else {
                for (auto & pair : layer.textures) {
                    auto & pos = pair.first;
                    auto & cube = *pair.second;
                    if (!cube.vertices.empty()) {
                        triangleVertices.clear();
                        textureVertices.clear();
                        for (const auto & vertex : cube.vertices) {
                            triangleVertices.push_back({{vertex.x, vertex.y, vertex.z * scale}});
                            const auto depthOffset = static_cast<float>(vertex.z - pos.z * gpucubeedge);
                            const auto texR = (0.5f + depthOffset) / gpucubeedge;
                            textureVertices.push_back({{static_cast<float>(vertex.x - pos.x * gpucubeedge) / gpucubeedge
                                                        , static_cast<float>(vertex.y - pos.y * gpucubeedge) / gpucubeedge
                                                        , texR}});
                        }
                        render(cube);
                    }
                }
            }
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_3D);

    raw_data_shader.disableAttributeArray(vertexLocation);
    raw_data_shader.disableAttributeArray(texLocation);
    raw_data_shader.release();
    overlay_data_shader.disableAttributeArray(overtexLocation);
    overlay_data_shader.disableAttributeArray(otexLocation);
    overlay_data_shader.release();

    times.recordSample();

//    qDebug() << "render time: " << times.waitForIntervals();
}

void ViewportOrtho::renderViewport(const RenderOptions &options) {
    glEnable(GL_MULTISAMPLE);

    float dataPxX = displayedIsoPx;
    float dataPxY = displayedIsoPx;

    const auto scale = Dataset::current().scale.componentMul(n).length();
    const auto nears = scale * state->viewerState->depthCutOff;
    const auto fars = -scale * state->viewerState->depthCutOff;;
    const auto nearVal = -nears;
    const auto farVal = -fars;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-displayedIsoPx, +displayedIsoPx, -displayedIsoPx, +displayedIsoPx, nearVal, farVal);// gluLookAt relies on an unaltered cartesian Projection

    const auto isoCurPos = Dataset::current().scales[0].componentMul(state->viewerState->currentPosition);
    auto view = [&](){
        glLoadIdentity();
        // place eye at the center so the depth cutoff is applied correctly
        const auto eye = Coord<double>(isoCurPos);
        // offset center with n at the same magnitude as the current position
        // gluLookAt only uses it to calculate the focal direction via center - camera
        // which suffers heavy loss of precision if the magnitudes differ substantially
        const auto center = Coord<double>(isoCurPos) - Coord<double>(n) * std::pow(10, std::log10(isoCurPos.length()));
        const auto v2 = Coord<double>(this->v2);
        gluLookAt(eye.x, eye.y, eye.z
                  , center.x, center.y, center.z
                  , v2.x, v2.y, v2.z);// negative up vectors, because origin is at the top
    };
    auto slice = [&](auto & texture, std::size_t layerId, floatCoordinate offset = {}){
        if (!options.nodePicking) {
            state->viewer->vpGenerateTexture(*this, layerId);
            glEnable(GL_TEXTURE_2D);
            texture.texHandle[layerId].bind();
            glPushMatrix();
            glTranslatef(isoCurPos.x + offset.x, isoCurPos.y + offset.y, isoCurPos.z + offset.z);
            glBegin(GL_QUADS);
                glNormal3i(n.x, n.y, n.z);
                glTexCoord2f(texture.texLUx, texture.texLUy);
                glVertex3f(-dataPxX * v1.x - dataPxY * v2.x,
                           -dataPxX * v1.y - dataPxY * v2.y,
                           -dataPxX * v1.z - dataPxY * v2.z);
                glTexCoord2f(texture.texRUx, texture.texRUy);
                glVertex3f( dataPxX * v1.x - dataPxY * v2.x,
                            dataPxX * v1.y - dataPxY * v2.y,
                            dataPxX * v1.z - dataPxY * v2.z);
                glTexCoord2f(texture.texRLx, texture.texRLy);
                glVertex3f( dataPxX * v1.x + dataPxY * v2.x,
                            dataPxX * v1.y + dataPxY * v2.y,
                            dataPxX * v1.z + dataPxY * v2.z);
                glTexCoord2f(texture.texLLx, texture.texLLy);
                glVertex3f(-dataPxX * v1.x + dataPxY * v2.x,
                           -dataPxX * v1.y + dataPxY * v2.y,
                           -dataPxX * v1.z + dataPxY * v2.z);
            glEnd();
            glPopMatrix();
            texture.texHandle[layerId].release();
            glDisable(GL_TEXTURE_2D);
        }
    };
    glMatrixMode(GL_MODELVIEW);
    view();
    updateFrustumClippingPlanes();

    glPolygonMode(GL_FRONT, GL_FILL);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // data that’s visible through the skeleton (e.g. halo)
    for (std::size_t i = 0; i < texture.texHandle.size(); ++i) {
        const auto ordered_i = state->viewerState->layerOrder[i];
        const auto & layerSettings = state->viewerState->layerRenderSettings[ordered_i];
        if (!options.nodePicking && layerSettings.visible && !Dataset::datasets[ordered_i].isOverlay()) {
            glColor4f(layerSettings.color.redF(), layerSettings.color.greenF(), layerSettings.color.blueF(), layerSettings.opacity);
            slice(texture, ordered_i, n * fars);// offset to the far clipping plane to avoid clipping the skeleton
            break;
        }
    }

    glColor4f(1, 1, 1, 1);
    if (options.drawSkeleton && state->viewerState->skeletonDisplayVPOrtho.testFlag(TreeDisplay::ShowInOrthoVPs)) {
        glPushMatrix();
        if (viewportType != VIEWPORT_ARBITRARY) {// arb already is at the pixel center
            const auto halfPixelOffset = 0.5 * (v1 - v2) * Dataset::current().scale;
            glTranslatef(halfPixelOffset.x(), halfPixelOffset.y(), halfPixelOffset.z());
        }
        renderSkeleton(options);
        glPopMatrix();
    }

    glDepthMask(GL_FALSE);// overlay shouldn’t prevent opaque layer rendering below the skeleton
    bool notFirst{false};
    for (std::size_t i = 0; i < texture.texHandle.size(); ++i) {
        const auto ordered_i = state->viewerState->layerOrder[i];
        const auto & layerSettings = state->viewerState->layerRenderSettings[ordered_i];
        if (!options.nodePicking && layerSettings.visible) {
            if (!Dataset::datasets[ordered_i].isOverlay()) {
                const auto alpha = (notFirst ? 1 : 0.6) * layerSettings.opacity;
                glColor4f(layerSettings.color.redF() * layerSettings.opacity, layerSettings.color.greenF() * layerSettings.opacity, layerSettings.color.blueF() * layerSettings.opacity, alpha);
                if (notFirst) {
                    glBlendFunc(GL_ONE, GL_ONE);// mix all non overlay channels
                    slice(texture, ordered_i, n * fars);// accumulate all opaque layers below the skeleton to not overdraw it
                } else {// second raw slice is semi transparent, with one direction of the skeleton showing through and the other rendered above
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    slice(texture, ordered_i);
                }
                notFirst = true;
            } else if (options.drawOverlay) {
                glColor4f(layerSettings.color.redF(), layerSettings.color.greenF(), layerSettings.color.blueF(), layerSettings.opacity);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                slice(texture, ordered_i);// overlay can be rendered inside the skeleton
            }
        }
    }
    glDepthMask(GL_TRUE);

    glColor4f(1, 1, 1, 1);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);// don’t render skeleton above crosshairs
    if (options.drawCrosshairs) {
        glPushMatrix();
        glTranslatef(isoCurPos.x, isoCurPos.y, isoCurPos.z);
        glLineWidth(1);
        const auto hOffset = viewportType == VIEWPORT_ARBITRARY ? QVector3D{} : 0.5 * v1 * Dataset::current().scale;
        const auto vOffset = viewportType == VIEWPORT_ARBITRARY ? QVector3D{} : 0.5 * v2 * Dataset::current().scale;
        glBegin(GL_LINES);
            glColor4f(std::abs(v2.z), std::abs(v2.y), std::abs(v2.x), 0.3);
            const auto halfLength = dataPxX * v1;
            const auto left = -halfLength - vOffset;
            const auto right = halfLength - vOffset;
            glVertex3f(left.x(), left.y(), left.z());
            glVertex3f(right.x(), right.y(), right.z());

            glColor4f(std::abs(v1.z), std::abs(v1.y), std::abs(v1.x), 0.3);
            const auto halfHeight = dataPxX * v2;
            const auto top = -halfHeight + hOffset;
            const auto bottom = halfHeight + hOffset;
            glVertex3f(top.x(), top.y(), top.z());
            glVertex3f(bottom.x(), bottom.y(), bottom.z());
        glEnd();
        glPopMatrix();
        glColor4f(1, 1, 1, 1);
    }
    auto setStateAndRenderMesh = [this](auto func){
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(-displayedIsoPx, +displayedIsoPx, -displayedIsoPx, +displayedIsoPx, -(0.5), -(-state->skeletonState->volBoundary()));
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        func();
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

    };
    if (options.meshPicking) {
        setStateAndRenderMesh([this](){ pickMeshIdAtPosition(); });
    } else if (state->viewerState->meshDisplay.testFlag(TreeDisplay::ShowInOrthoVPs) && options.drawMesh) {
        QOpenGLFramebufferObjectFormat format;
        format.setSamples(0);//state->viewerState->sampleBuffers
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        std::array<GLint, 4> vp;
        glGetIntegerv(GL_VIEWPORT, vp.data());
        QOpenGLFramebufferObject fbo(vp[2], vp[2], format);
        fbo.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?!?!?!?

        setStateAndRenderMesh([this](){ renderMesh(); });
        if (auto snapshotFboPtr = snapshotFbo.lock()) {
            snapshotFboPtr->bind();
        } else {
            QOpenGLFramebufferObject::bindDefault();
        }

        std::vector<floatCoordinate> vertices{
                isoCurPos - dataPxX * v1 - dataPxY * v2,
                isoCurPos + dataPxX * v1 - dataPxY * v2,
                isoCurPos + dataPxX * v1 + dataPxY * v2,
                isoCurPos - dataPxX * v1 + dataPxY * v2
        };
        std::vector<float> texCoordComponents{0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glBindTexture(GL_TEXTURE_2D, fbo.texture());
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());
        glTexCoordPointer(2, GL_FLOAT, 0, texCoordComponents.data());

        glDrawArrays(GL_QUADS, 0, 4);

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_TEXTURE_2D);
    }
    if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Brush) && hasCursor) {
        glPushMatrix();
        view();
        renderBrush(getMouseCoordinate());
        glPopMatrix();
    }
}

void Viewport3D::renderVolumeVP() {
    auto& seg = Segmentation::singleton();

    std::array<double, 3> background_color;
    seg.volume_background_color.getRgbF(&background_color[0], &background_color[1], &background_color[2]);
    glClearColor(background_color[0], background_color[1], background_color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(seg.volume_tex_id != 0) {
        static float volumeClippingAdjust = 1.73f;
        static float translationSpeedAdjust = 1.0 / 500.0f;
        auto cubeLen = Dataset::current().cubeEdgeLength;
        int texLen = seg.volume_tex_len;
        GLuint volTexId = seg.volume_tex_id;

        static Profiler render_profiler;

        render_profiler.start(); // ----------------------------------------------------------- profiling

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f);

        // volume viewport rotation
        static QMatrix4x4 volRotMatrix;
        float rotdx = state->skeletonState->rotdx;
        float rotdy = state->skeletonState->rotdy;
        state->skeletonState->rotdx = 0;
        state->skeletonState->rotdy = 0;

        if(rotdx || rotdy) {
            QVector3D xRotAxis{0.0f, 1.0f, 0.0f};
            QVector3D yRotAxis{1.0f, 0.0f, 0.0f};

            volRotMatrix.rotate(-rotdx, xRotAxis);
            volRotMatrix.rotate( rotdy, yRotAxis);
        }

        // volume viewport translation
        static float transx = 0.0f;
        static float transy = 0.0f;
        transx += seg.volume_mouse_move_x * translationSpeedAdjust;
        transy += seg.volume_mouse_move_y * translationSpeedAdjust;
        seg.volume_mouse_move_x = 0;
        seg.volume_mouse_move_y = 0;

        // volume viewport zoom
        static float zoom = seg.volume_mouse_zoom;
        zoom = seg.volume_mouse_zoom;

        // dataset scaling adjustment
        auto datascale = Dataset::current().scale;
        float biggestScale = 0.0f;
        if(datascale.x > datascale.y) {
            biggestScale = datascale.x;
        } else {
            biggestScale = datascale.y;
        }
        if(datascale.z > biggestScale) {
            biggestScale = datascale.z;
        }

        float smallestScale = 0.0f;
        if(datascale.x < datascale.y) {
            smallestScale = datascale.x;
        } else {
            smallestScale = datascale.y;
        }
        if(datascale.z < smallestScale) {
            smallestScale = datascale.z;
        }
        float maxScaleRatio = biggestScale / smallestScale;
        float scalex = 1.0f / (datascale.x / biggestScale);
        float scaley = 1.0f / (datascale.y / biggestScale);
        float scalez = 1.0f / (datascale.z / biggestScale);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        // dataset translation adjustment
        glTranslatef((static_cast<float>(state->viewerState->currentPosition.x % cubeLen) / cubeLen - 0.5f) / state->M,
                     (static_cast<float>(state->viewerState->currentPosition.y % cubeLen) / cubeLen - 0.5f) / state->M,
                     (static_cast<float>(state->viewerState->currentPosition.z % cubeLen) / cubeLen - 0.5f) / state->M);

        glTranslatef(0.5f, 0.5f, 0.5f);
        glScalef(volumeClippingAdjust, volumeClippingAdjust, volumeClippingAdjust); // scale to remove cube corner clipping
        glScalef(scalex, scaley, scalez); // dataset scaling adjustment
        glMultMatrixf(volRotMatrix.data()); // volume viewport rotation
        glScalef(1.0f/zoom, 1.0f/zoom, 1.0f/zoom*2.0f); // volume viewport zoom
        glTranslatef(-0.5f, -0.5f, -0.5f);
        glTranslatef(transx, transy, 0.0f); // volume viewport translation

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_3D);

        glBindTexture(GL_TEXTURE_3D, volTexId);
        float volume_opacity = seg.volume_opacity / 255.0f;
        for(int i = 0; i < texLen * volumeClippingAdjust * maxScaleRatio; ++i) {
            float depth = i/(texLen * volumeClippingAdjust * maxScaleRatio);
            glColor4f(depth, depth, depth, volume_opacity);
            glBegin(GL_QUADS);
                glTexCoord3f(0.0f, 1.0f, depth);
                glVertex3f(-1.0f, -1.0f,  1.0f-depth*2.0f);
                glTexCoord3f(1.0f, 1.0f, depth);
                glVertex3f( 1.0f, -1.0f,  1.0f-depth*2.0f);
                glTexCoord3f(1.0f, 0.0f, depth);
                glVertex3f( 1.0f,  1.0f,  1.0f-depth*2.0f);
                glTexCoord3f(0.0f, 0.0f, depth);
                glVertex3f(-1.0f,  1.0f,  1.0f-depth*2.0f);
            glEnd();
        }

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);

        // Reset previously changed OGL parameters
        glDisable(GL_TEXTURE_3D);
        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        render_profiler.end(); // ----------------------------------------------------------- profiling

        // --------------------- display some profiling information ------------------------
        // static auto timer = std::chrono::steady_clock::now();
        // std::chrono::duration<double> duration = std::chrono::steady_clock::now() - timer;
        // if(duration.count() > 1.0) {
        //     qDebug() << "render  avg time: " <<  render_profiler.average_time()*1000 << "ms";
        //     qDebug() << "---------------------------------------------";

        //     timer = std::chrono::steady_clock::now();
        // }
    }
}

void Viewport3D::renderViewport(const RenderOptions &options) {
    auto& seg = Segmentation::singleton();
    if (seg.volume_render_toggle) {
        if (!options.nodePicking) {
            if(seg.volume_update_required) {
                seg.volume_update_required = false;
                updateVolumeTexture();
            }
            renderVolumeVP();
        }
    } else {
        renderSkeletonVP(options);
    }
}

void ViewportBase::renderMeshBuffer(Mesh & buf, const bool picking) {
    if (picking) {
        glDisable(GL_BLEND);
    }
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(0.5, 0.5, 0.5);

    // get modelview and projection matrices
    GLfloat modelview_mat[4][4];
    glGetFloatv(GL_MODELVIEW_MATRIX, &modelview_mat[0][0]);
    GLfloat projection_mat[4][4];
    glGetFloatv(GL_PROJECTION_MATRIX, &projection_mat[0][0]);

    auto & meshShader = picking ? meshIdShader
                        : buf.useTreeColor ? meshTreeColorShader
                        : this->meshShader;

    meshShader.bind();
    meshShader.setUniformValue("modelview_matrix", modelview_mat);
    meshShader.setUniformValue("projection_matrix", projection_mat);
    floatCoordinate normal = {0, 0, 0};
    const bool isMeshSlicing = viewportType != VIEWPORT_SKELETON;
    if (isMeshSlicing) {
        normal = state->mainWindow->viewportOrtho(viewportType)->n;
    }
    meshShader.setUniformValue("vp_normal", normal.x, normal.y, normal.z);
    const float alphaFactor = isMeshSlicing ? state->viewerState->meshAlphaFactorSlicing : state->viewerState->meshAlphaFactor3d;
    if (!picking) {
        meshShader.setUniformValue("alpha_factor", alphaFactor);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    buf.position_buf.bind();
    int vertexLocation = meshShader.attributeLocation("vertex");
    meshShader.enableAttributeArray(vertexLocation);
    meshShader.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3);
    buf.position_buf.release();

    buf.normal_buf.bind();
    int normalLocation = meshShader.attributeLocation("normal");
    meshShader.enableAttributeArray(normalLocation);
    meshShader.setAttributeBuffer(normalLocation, GL_FLOAT, 0, 3);
    buf.normal_buf.release();
    const bool disableColorLocation = picking || !buf.useTreeColor;
    int colorLocation = meshShader.attributeLocation("color");
    if (disableColorLocation) {
        auto & correct_color_buf = picking ? buf.picking_color_buf : buf.color_buf;
        correct_color_buf.bind();
        meshShader.enableAttributeArray(colorLocation);
        meshShader.setAttributeBuffer(colorLocation, GL_UNSIGNED_BYTE, 0, 4);
        correct_color_buf.release();
    }
    if (!picking) {
        QColor color = buf.correspondingTree->color;
        if (state->viewerState->highlightActiveTree && buf.correspondingTree == state->skeletonState->activeTree) {
            color = Qt::red;
        }
        color.setAlpha(color.alpha() * alphaFactor);
        meshShader.setUniformValue("tree_color", color);
    }
    if(buf.index_count != 0) {
        buf.index_buf.bind();
        glDrawElements(buf.render_mode, buf.index_count, GL_UNSIGNED_INT, 0);
        buf.index_buf.release();
    } else {
        glDrawArrays(buf.render_mode, 0, buf.vertex_count);
    }
    if (disableColorLocation) {
        meshShader.disableAttributeArray(colorLocation);
    }
    meshShader.disableAttributeArray(normalLocation);
    meshShader.disableAttributeArray(vertexLocation);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    meshShader.release();
    glPopMatrix();
    glEnable(GL_BLEND);
}

static bool shouldRenderMesh(const treeListElement & tree, const ViewportType viewportType) {
    const bool validMesh = tree.mesh && tree.mesh->vertex_count > 0;
    const auto displayFlags = (viewportType == VIEWPORT_SKELETON) ? state->viewerState->skeletonDisplayVP3D : state->viewerState->skeletonDisplayVPOrtho;
    const bool showFilter = ((viewportType == VIEWPORT_SKELETON) && displayFlags.testFlag(TreeDisplay::ShowIn3DVP)) || displayFlags.testFlag(TreeDisplay::ShowInOrthoVPs);
    const bool selectionFilter = !displayFlags.testFlag(TreeDisplay::OnlySelected) || tree.selected;
    return tree.render && showFilter && selectionFilter && validMesh;
}

void ViewportBase::renderMesh() {
    std::vector<std::reference_wrapper<Mesh>> translucentMeshes;
    for (const auto & tree : state->skeletonState->trees) {
        if (shouldRenderMesh(tree, viewportType)) {
            const auto hasTranslucentFirstVertexColor = [](Mesh & mesh){
                mesh.color_buf.bind();
                if (mesh.color_buf.size() < 4) {
                    throw std::runtime_error("non tree color mesh has no colors");
                }
                std::uint8_t buffer;
                mesh.color_buf.read(3, &buffer, sizeof (buffer));
                mesh.color_buf.release();
                return buffer < 255;
            };
            if ((tree.mesh->useTreeColor && tree.color.alphaF() < 1.0) || (!tree.mesh->useTreeColor && hasTranslucentFirstVertexColor(*(tree.mesh)))) {
                translucentMeshes.emplace_back(*(tree.mesh));
            } else {
                renderMeshBuffer(*(tree.mesh));
            }
        }
    }
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    for (const auto & mesh : translucentMeshes) {// render translucent after opaque meshes
        renderMeshBuffer(mesh);
    }
    glDisable(GL_CULL_FACE);
}

uint32_t meshColorToId(const QColor & color) {
    return color.red() + (color.green() << 8) + (color.blue() << 16) + (color.alpha() << 24);
}

std::array<unsigned char, 4> meshIdToColor(uint32_t id) {
    return {{static_cast<unsigned char>(id),
             static_cast<unsigned char>(id >> 8),
             static_cast<unsigned char>(id >> 16),
             static_cast<unsigned char>(id >> 24)}};
}

boost::optional<BufferSelection> ViewportBase::pickMesh(const QPoint pos) {
    makeCurrent();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, this->width(), this->height());
    QOpenGLFramebufferObject fbo(width(), height(), QOpenGLFramebufferObject::CombinedDepthStencil);
    fbo.bind();
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Qt does not clear it?
    renderViewport(RenderOptions::meshPickingRenderOptions());
    fbo.release();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glPopAttrib();
    // read color and translate to id
    QImage fboImage(fbo.toImage());
    // Need to specify image format with no premultiplied alpha.
    // Otherwise the image is automatically unpremultiplied on pixel read even though it was never premultiplied in the first place. See https://doc.qt.io/qt-5/qopenglframebufferobject.html#toImage
    QImage image(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_ARGB32);
    const auto triangleID = meshColorToId(image.pixelColor(pos));
    boost::optional<treeListElement&> treeIt;
    for (auto & tree : state->skeletonState->trees) {// find tree with appropriate triangle range
        if (tree.mesh && tree.mesh->pickingIdOffset + tree.mesh->vertex_count > triangleID) {
            treeIt = tree;
            break;
        }
    }
    floatCoordinate coord;
    if (treeIt) {
        const auto index = triangleID - treeIt->mesh->pickingIdOffset;

        std::array<GLfloat, 3> vertex_components;
        treeIt->mesh->position_buf.bind();
        treeIt->mesh->position_buf.read(index * sizeof(vertex_components), vertex_components.data(), vertex_components.size() * sizeof(vertex_components[0]));
        treeIt->mesh->position_buf.release();

        coord = floatCoordinate{vertex_components[0], vertex_components[1], vertex_components[2]};
        coord  /= Dataset::current().scales[0];
        if (viewportType != VIEWPORT_SKELETON) {
            // project point onto ortho plane. Necessary, because in reality user clicks a triangle behind the plane.
            auto * vp = state->mainWindow->viewportOrtho(viewportType);
            auto distVec = coord - state->viewerState->currentPosition;
            auto dist = distVec.dot(vp->n);
            coord = coord - dist * vp->n;
        }
    }
    return treeIt ? boost::optional<BufferSelection>{{treeIt->treeID, coord}} : boost::none;
}

void ViewportBase::pickMeshIdAtPosition() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//the depth thing buffer clear is the important part

    // create id map
    std::uint32_t id_counter = 1;
    for (auto & tree : state->skeletonState->trees) {
        if (!tree.mesh || tree.mesh->render_mode != GL_TRIANGLES) {// can’t pick GL_POINTS
            continue;
        }
        tree.mesh->picking_color_buf.bind();
        const auto pickingBufferFilled = tree.mesh->picking_color_buf.size() == static_cast<int>(tree.mesh->vertex_count * 4 * sizeof(GLubyte)); // > 0 not sufficient, e.g. after a merge we have fewer colors than vertices
        const auto pickingMeshValid = id_counter == tree.mesh->pickingIdOffset && pickingBufferFilled;
        if (pickingMeshValid) {// increment
            id_counter += tree.mesh->vertex_count;
        } else {// create picking color buf and increment
            tree.mesh->pickingIdOffset = id_counter;

            std::vector<std::array<GLubyte, 4>> picking_colors;
            for (std::size_t i{0}; i < tree.mesh->vertex_count; ++i) {// for each vertex
                picking_colors.emplace_back(meshIdToColor(id_counter++));
            }
            tree.mesh->picking_color_buf.allocate(picking_colors.data(), picking_colors.size() * sizeof(picking_colors[0]));
        }
        tree.mesh->picking_color_buf.release();
        if (shouldRenderMesh(tree, viewportType)) {
            renderMeshBuffer(*tree.mesh, true);
        }
    }
}

void Viewport3D::renderSkeletonVP(const RenderOptions &options) {
    glEnable(GL_MULTISAMPLE);
    const auto scaledBoundary = Dataset::current().scales[0].componentMul(Dataset::current().boundary);

    auto rotateMe = [this, scaledBoundary](auto x, auto y){
        floatCoordinate rotationCenter{Dataset::current().scales[0].componentMul(state->viewerState->currentPosition)};
        if (state->viewerState->rotationCenter == RotationCenter::ActiveNode && state->skeletonState->activeNode != nullptr) {
            rotationCenter = Dataset::current().scales[0].componentMul(state->skeletonState->activeNode->position);
        } else if (state->viewerState->rotationCenter == RotationCenter::DatasetCenter) {
            rotationCenter = scaledBoundary / 2;
        }
        // calculate inverted rotation
        std::array<float, 16> inverseRotation;
        rotation.inverted().copyDataTo(inverseRotation.data());
        // invert rotation
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(state->skeletonState->skeletonVpModelView);
        glTranslatef(rotationCenter.x, rotationCenter.y, rotationCenter.z);
        glMultMatrixf(inverseRotation.data());
        // add new rotation
        QMatrix4x4 singleRotation;
        singleRotation.rotate(y, {0, 1, 0});
        singleRotation.rotate(x, {1, 0, 0});
        std::array<float, 16> rotationState;
        (rotation *= singleRotation).copyDataTo(rotationState.data()); // transforms to row-major matrix
        // apply complete rotation
        glMultMatrixf(rotationState.data());
        glTranslatef(-rotationCenter.x, -rotationCenter.y, -rotationCenter.z);
        // save the modified basic model view matrix
        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);
    };

    // perform user defined coordinate system rotations. use single matrix multiplication as opt.! TDitem
    if (state->skeletonState->rotdx != 0 || state->skeletonState->rotdy != 0) {
        rotateMe(-state->skeletonState->rotdy, state->skeletonState->rotdx);// moving the cursor horizontally rotates the y axis
        state->skeletonState->rotdx = state->skeletonState->rotdy = 0;
    }

    const auto xy = state->skeletonState->definedSkeletonVpView == SKELVP_XY_VIEW;
    const auto xz = state->skeletonState->definedSkeletonVpView == SKELVP_XZ_VIEW;
    const auto zy = state->skeletonState->definedSkeletonVpView == SKELVP_ZY_VIEW;
    if (xy || xz || zy) {
        state->skeletonState->definedSkeletonVpView = SKELVP_CUSTOM;

        rotation.setToIdentity();
        QMatrix4x4{}.copyDataTo(state->skeletonState->skeletonVpModelView);
        const auto previousRotation = state->viewerState->rotationCenter;
        state->viewerState->rotationCenter = RotationCenter::CurrentPosition;
        rotateMe(90 * xz, -90 * zy);// updates rotationState and skeletonVpModelView, must therefore also be called for xy
        state->viewerState->rotationCenter = previousRotation;

        const auto translate = Dataset::current().scales[0].componentMul(state->viewerState->currentPosition);
        translateX = translate.x;
        translateY = translate.y;
    } else if (state->skeletonState->definedSkeletonVpView == SKELVP_R90 || state->skeletonState->definedSkeletonVpView == SKELVP_R180) {
        state->skeletonState->rotdx = 10;
        state->skeletonState->rotationcounter++;
        if (state->skeletonState->rotationcounter > (state->skeletonState->definedSkeletonVpView == SKELVP_R90 ? 9 : 18)) {
            state->skeletonState->rotdx = 0;
            state->skeletonState->definedSkeletonVpView = SKELVP_CUSTOM;
            state->skeletonState->rotationcounter = 0;
        }

    } else if (state->skeletonState->definedSkeletonVpView == SKELVP_RESET) {
        state->skeletonState->definedSkeletonVpView = SKELVP_CUSTOM;
        translateX = 0;
        translateY = 0;
        zoomFactor = 1;
        emit updateZoomWidget();

        rotation.setToIdentity();
        QMatrix4x4{}.copyDataTo(state->skeletonState->skeletonVpModelView);
        const auto previousRotationCenter = state->viewerState->rotationCenter;
        state->viewerState->rotationCenter = RotationCenter::DatasetCenter;
        rotateMe(90, 0);
        rotateMe(0, -15);
        rotateMe(-15, 0);
        state->viewerState->rotationCenter = previousRotationCenter;
        translateX = 0.5 * scaledBoundary.x;
        translateY = 0.5 * scaledBoundary.y;
    }

    const auto zoomedHalfBoundary = 0.5 * state->skeletonState->volBoundary() / zoomFactor;

    std::array<GLint, 4> vp;
    glGetIntegerv(GL_VIEWPORT, vp.data());
    edgeLength = vp[2]/devicePixelRatio(); // retrieve adjusted size for snapshot
    displayedlengthInNmX = 2.0 * zoomedHalfBoundary;
    screenPxXPerDataPx = edgeLength / displayedlengthInNmX;
    const auto left = translateX - zoomedHalfBoundary;
    const auto right = translateX + zoomedHalfBoundary;
    const auto bottom = translateY + zoomedHalfBoundary;
    const auto top = translateY - zoomedHalfBoundary;
    const auto nears = -state->skeletonState->volBoundary();
    const auto fars = state->skeletonState->volBoundary();
    const auto nearVal = -nears;
    const auto farVal = -fars;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, nearVal, farVal);

    // Now we set up the view on the skeleton and draw some very basic VP stuff like the gray background
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // load model view matrix that stores rotation state!
    glLoadMatrixf(state->skeletonState->skeletonVpModelView);
    if (!options.nodePicking) {
        // Now we draw the  background of our skeleton VP
        glClearColor(1, 1, 1, 1);// white
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (options.drawViewportPlanes) { // Draw the slice planes for orientation inside the data stack
        glPushMatrix();

        const auto isoCurPos = Dataset::current().scales[0].componentMul(state->viewerState->currentPosition);
        glTranslatef(isoCurPos.x, isoCurPos.y, isoCurPos.z);
        // raw slice image
        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1., 1., 1., 1.);
        if (state->viewerState->showVpPlanes) {
            if (state->viewerState->showXYplane && state->viewer->window->viewportXY->isVisible()) {
                renderArbitrarySlicePane(*state->viewer->window->viewportXY, options);
            }
            if (state->viewerState->showXZplane && state->viewer->window->viewportXZ->isVisible()) {
                renderArbitrarySlicePane(*state->viewer->window->viewportXZ, options);
            }
            if (state->viewerState->showZYplane && state->viewer->window->viewportZY->isVisible()) {
                renderArbitrarySlicePane(*state->viewer->window->viewportZY, options);
            }
            if (state->viewerState->showArbplane && state->viewer->window->viewportArb->isVisible()) {
                renderArbitrarySlicePane(*state->viewer->window->viewportArb, options);
            }
        }
        // colored slice boundaries
        if (options.vp3dSliceBoundaries) {
            glDisable(GL_TEXTURE_2D);
            state->viewer->window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
                if (orthoVP.isVisible()) {
                    const float dataPxX = orthoVP.displayedIsoPx;
                    const float dataPxY = orthoVP.displayedIsoPx;
                    glColor4f(0.7 * std::abs(orthoVP.n.z), 0.7 * std::abs(orthoVP.n.y), 0.7 * std::abs(orthoVP.n.x), 1.);
                    glBegin(GL_LINE_LOOP);
                        glVertex3f(-dataPxX * orthoVP.v1.x - dataPxY * orthoVP.v2.x,
                                   -dataPxX * orthoVP.v1.y - dataPxY * orthoVP.v2.y,
                                   -dataPxX * orthoVP.v1.z - dataPxY * orthoVP.v2.z);
                        glVertex3f( dataPxX * orthoVP.v1.x - dataPxY * orthoVP.v2.x,
                                    dataPxX * orthoVP.v1.y - dataPxY * orthoVP.v2.y,
                                    dataPxX * orthoVP.v1.z - dataPxY * orthoVP.v2.z);
                        glVertex3f( dataPxX * orthoVP.v1.x + dataPxY * orthoVP.v2.x,
                                    dataPxX * orthoVP.v1.y + dataPxY * orthoVP.v2.y,
                                    dataPxX * orthoVP.v1.z + dataPxY * orthoVP.v2.z);
                        glVertex3f(-dataPxX * orthoVP.v1.x + dataPxY * orthoVP.v2.x,
                                   -dataPxX * orthoVP.v1.y + dataPxY * orthoVP.v2.y,
                                   -dataPxX * orthoVP.v1.z + dataPxY * orthoVP.v2.z);
                    glEnd();
                }
            });
        }
        // intersection lines
        if (options.vp3dSliceIntersections) {
            const auto size = state->viewer->window->viewportXY->displayedIsoPx;
            glColor4f(0., 0., 0., 1.);
            if (!state->viewerState->showXYplane || !state->viewerState->showXZplane) {
                glBegin(GL_LINES);
                    glVertex3f(-size, 0, 0);
                    glVertex3f( size, 0, 0);
                glEnd();
            }
            if (!state->viewerState->showXYplane || !state->viewerState->showZYplane) {
                glBegin(GL_LINES);
                    glVertex3f(0, -size, 0);
                    glVertex3f(0,  size, 0);
                glEnd();
            }
            if (!state->viewerState->showXZplane || !state->viewerState->showZYplane) {
                glBegin(GL_LINES);
                    glVertex3f(0, 0, -size);
                    glVertex3f(0, 0,  size);
                glEnd();
            }
        }
        glPopMatrix();
    }

    if (options.drawBoundaryBox || options.drawBoundaryAxes) {
        // Now we draw the dataset corresponding stuff (volume box of right size, axis descriptions...)
        if(options.drawBoundaryBox) {
            glPushMatrix();
            glScalef(scaledBoundary.x, scaledBoundary.y, scaledBoundary.z);

            glColor4f(0.8, 0.8, 0.8, 1.0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glBegin(GL_QUADS);
                glNormal3i(0, 0, 1);// low z
                glVertex3i(0, 0, 0);
                glVertex3i(1, 0, 0);

                glVertex3i(1, 1, 0);
                glVertex3i(0, 1, 0);

                glNormal3i(0, 0, 1);// high z
                glVertex3i(0, 0, 1);
                glVertex3i(1, 0, 1);

                glVertex3i(1, 1, 1);
                glVertex3i(0, 1, 1);

                glNormal3i(0, 1, 0);// low y
                glVertex3i(0, 0, 0);
                glVertex3i(0, 0, 1);

                glVertex3i(1, 0, 1);
                glVertex3i(1, 0, 0);

                glNormal3i(0, 1, 0);// high y
                glVertex3i(0, 1, 0);
                glVertex3i(0, 1, 1);

                glVertex3i(1, 1, 1);
                glVertex3i(1, 1, 0);

                glNormal3i(1, 0, 0);// low x
                glVertex3i(0, 0, 0);
                glVertex3i(0, 0, 1);

                glVertex3i(0, 1, 1);
                glVertex3i(0, 1, 0);

                glNormal3i(1, 0, 0);// high x
                glVertex3i(1, 0, 0);
                glVertex3i(1, 0, 1);

                glVertex3i(1, 1, 1);
                glVertex3i(1, 1, 0);
            glEnd();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glPopMatrix();
        }

        // draw ground grid
        if(options.drawBoundaryBox) {
            auto scalebarLenNm = std::get<1>(getScaleBarLabelNmAndPx(displayedlengthInNmX, edgeLength));
            if (scalebarLenNm == 0.0) {
                scalebarLenNm = Dataset::current().boundary.x * Dataset::current().scales[0].x / 10.0;
            }
            const auto grid_max_x = Dataset::current().boundary.x * Dataset::current().scales[0].x;
            const auto grid_spacing_x = scalebarLenNm;
            const auto grid_max_y = Dataset::current().boundary.y * Dataset::current().scales[0].y;
            const auto grid_spacing_y = scalebarLenNm;

            glPushMatrix();
            glScalef(scaledBoundary.x, scaledBoundary.y, scaledBoundary.z);

            glColor4d(0.85, 0.85, 0.85, 1.0);

            glLineWidth(1.0f);
            glBegin(GL_LINES);
                glNormal3i(0, 0, 1);// high z
                for(float i = 0; i < grid_max_x; i += grid_spacing_x) {
                    glVertex3f(i / grid_max_x, 0, 1);
                    glVertex3f(i / grid_max_x, 1, 1);
                }

                for(float i = 0; i < grid_max_y; i += grid_spacing_y) {
                    glVertex3f(0, i / grid_max_y, 1);
                    glVertex3f(1, i / grid_max_y, 1);
                }
            glEnd();

            glPopMatrix();
        }

        // draw axes
        auto renderAxis = [this, options](const floatCoordinate & targetView, const QString label) {
            glPushMatrix();

            floatCoordinate currentView = {0, 0, 1};
            const auto angle = std::acos(currentView.dot(targetView));
            auto axis = currentView.cross(targetView);
            if (axis.normalize()) {
                glRotatef(angle * 180/boost::math::constants::pi<float>(), axis.x, axis.y, axis.z);
            }
            const auto diameter = (state->skeletonState->volBoundary() / zoomFactor) * 5e-3;
            const auto coneDiameter = 3 * diameter;
            const auto coneLength = 6 * diameter;
            const auto length = targetView.length() - coneLength;
            // axis cylinder lid
            GLUquadricObj * gluFloorObj = gluNewQuadric();
            gluQuadricNormals(gluFloorObj, GLU_SMOOTH);
            gluDisk(gluFloorObj, 0, diameter, 10, 1);
            gluDeleteQuadric(gluFloorObj);
            // axis cylinder
            GLUquadricObj * gluCylObj = gluNewQuadric();
            gluQuadricNormals(gluCylObj, GLU_SMOOTH);
            gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
            gluCylinder(gluCylObj, diameter, diameter, length, 10, 1);
            gluDeleteQuadric(gluCylObj);

            glTranslatef(0, 0, length);
            // cone lid
            GLUquadricObj * gluLidObj = gluNewQuadric();
            gluQuadricNormals(gluLidObj, GLU_SMOOTH);
            gluDisk(gluLidObj, 0, coneDiameter, 10, 1);
            gluDeleteQuadric(gluLidObj);
            // cone
            gluCylObj = gluNewQuadric();
            gluQuadricNormals(gluCylObj, GLU_SMOOTH);
            gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
            gluCylinder(gluCylObj, coneDiameter, 0., coneLength, 10, 5);
            gluDeleteQuadric(gluCylObj);

            glPopMatrix();

            const auto offset = targetView + std::ceil(9 * diameter);
            renderText(offset, label, options.enableTextScaling);
        };
        glColor4f(0, 0, 0, 1);
        if (options.drawBoundaryAxes) {
            if (Viewport3D::showBoundariesInUm) {
                renderAxis(floatCoordinate(scaledBoundary.x, 0, 0), QString("x: %1 µm").arg(scaledBoundary.x * 1e-3));
                renderAxis(floatCoordinate(0, scaledBoundary.y, 0), QString("y: %1 µm").arg(scaledBoundary.x * 1e-3));
                renderAxis(floatCoordinate(0, 0, scaledBoundary.z), QString("z: %1 µm").arg(scaledBoundary.x * 1e-3));
            } else {
                renderAxis(floatCoordinate(scaledBoundary.x, 0, 0), QString("x: %1 px").arg(Dataset::current().boundary.x + state->skeletonState->displayMatlabCoordinates));
                renderAxis(floatCoordinate(0, scaledBoundary.y, 0), QString("y: %1 px").arg(Dataset::current().boundary.y + state->skeletonState->displayMatlabCoordinates));
                renderAxis(floatCoordinate(0, 0, scaledBoundary.z), QString("z: %1 px").arg(Dataset::current().boundary.z + state->skeletonState->displayMatlabCoordinates));
            }
        }
    }

    if (options.drawSkeleton && state->viewerState->skeletonDisplayVP3D.testFlag(TreeDisplay::ShowIn3DVP)) {
        glPushMatrix();
        updateFrustumClippingPlanes();// should update on vp view translate, rotate or scale
        renderSkeleton(options);
        glPopMatrix();
    }

    if (options.meshPicking) {
        pickMeshIdAtPosition();
    } else if (state->viewerState->meshDisplay.testFlag(TreeDisplay::ShowIn3DVP) && options.drawMesh) {
        renderMesh();
    }

    // Reset previously changed OGL parameters
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glLoadIdentity();
}

void ViewportOrtho::renderBrush(const Coordinate coord) {
    glPushMatrix();
    glLineWidth(2.0f);

    auto & seg = Segmentation::singleton();
    auto drawCursor = [this, &seg, coord](const float r, const float g, const float b) {
        const auto bradius = seg.brush.getRadius();
        const auto bview = seg.brush.getView();
        const auto xsize = bradius / Dataset::current().scales[0].x;
        const auto ysize = bradius / Dataset::current().scales[0].y;
        const auto zsize = bradius / Dataset::current().scales[0].z;

        //move from center to cursor
        glTranslatef(Dataset::current().scales[0].x * coord.x, Dataset::current().scales[0].y * coord.y, Dataset::current().scales[0].z * coord.z);
        if (viewportType == VIEWPORT_XZ && bview == brush_t::view_t::xz) {
            glTranslatef(0, 0, Dataset::current().scale.z);//move origin to other corner of voxel, idrk why that’s necessary
            glRotatef(-90, 1, 0, 0);
        } else if(viewportType == VIEWPORT_ZY && bview == brush_t::view_t::zy) {
            glTranslatef(0, 0, Dataset::current().scale.z);//move origin to other corner of voxel, idrk why that’s necessary
            glRotatef( 90, 0, 1, 0);
        } else if (viewportType != VIEWPORT_XY || bview != brush_t::view_t::xy) {
            return;
        }

        const bool xy = viewportType == VIEWPORT_XY;
        const bool xz = viewportType == VIEWPORT_XZ;
        const int z = 0;
        std::vector<floatCoordinate> vertices;
        if(seg.brush.getShape() == brush_t::shape_t::angular) {
            const auto x = xy || xz ? xsize : zsize;
            const auto y = xz ? zsize : ysize;
            //integer coordinates to round to voxel boundaries
            vertices.emplace_back(-x    , -y    , z);
            vertices.emplace_back( x + 1, -y    , z);
            vertices.emplace_back( x + 1,  y + 1, z);
            vertices.emplace_back(-x    ,  y + 1, z);
        } else if(seg.brush.getShape() == brush_t::shape_t::round) {
            const int xmax = xy ? xsize : xz ? xsize : zsize;
            const int ymax = xy ? ysize : xz ? zsize : ysize;
            int y = 0;
            int x = xmax;
            auto addVerticalPixelBorder = [&vertices](float x, float y, float z) {
                vertices.emplace_back(x, y    , z);
                vertices.emplace_back(x, y + 1, z);
            };
            auto addHorizontalPixelBorder = [&vertices](float x, float y, float z) {
                vertices.emplace_back(x    , y, z);
                vertices.emplace_back(x + 1, y, z);
            };
            while (x >= y) { //first part of the ellipse (circle with anisotropic pixels), y dominant movement
                auto val = isInsideSphere(xy ? x : xz ? x : z, xy ? y : xz ? z : y, xy ? z : xz ? y : x, bradius);
                if (val) {
                    addVerticalPixelBorder( x + 1,  y, z);
                    addVerticalPixelBorder(-x    ,  y, z);
                    addVerticalPixelBorder(-x    , -y, z);
                    addVerticalPixelBorder( x + 1, -y, z);
                } else if (x != xmax || y != 0) {
                    addHorizontalPixelBorder( x,  y    , z);
                    addHorizontalPixelBorder(-x,  y    , z);
                    addHorizontalPixelBorder(-x, -y + 1, z);
                    addHorizontalPixelBorder( x, -y + 1, z);
                }
                if (val) {
                    ++y;
                } else {
                    --x;
                }
            }

            x = 0;
            y = ymax;
            while (y >= x) { //second part of the ellipse, x dominant movement
                auto val = isInsideSphere(xy ? x : xz ? x : z, xy ? y : xz ? z : y, xy ? z : xz ? y : x, bradius);
                if (val) {
                    addHorizontalPixelBorder( x,  y + 1, z);
                    addHorizontalPixelBorder(-x,  y + 1, z);
                    addHorizontalPixelBorder(-x, -y    , z);
                    addHorizontalPixelBorder( x, -y    , z);
                } else if (y != ymax || x != 0) {
                    addVerticalPixelBorder( x    ,  y, z);
                    addVerticalPixelBorder(-x + 1,  y, z);
                    addVerticalPixelBorder(-x + 1, -y, z);
                    addVerticalPixelBorder( x    , -y, z);
                }
                if (val) {
                    ++x;
                } else {
                    --y;
                }
            }
        }
        // scale
        for (auto && point : vertices) {
            point.x *= Dataset::current().scales[0].componentMul(v1).length();
            point.y *= Dataset::current().scales[0].componentMul(v2).length();
        }
        // sort by angle
        const auto center = std::accumulate(std::begin(vertices), std::end(vertices), floatCoordinate(0, 0, 0)) / vertices.size();
        const auto start = vertices.front() - center;
        std::sort(std::begin(vertices), std::end(vertices), [&center, &start](const floatCoordinate & lhs, const floatCoordinate & rhs) {
            const auto lineLhs = lhs - center;
            const auto lineRhs = rhs - center;
            return std::atan2(start.x * lineLhs.y - start.y * lineLhs.x, start.dot(lineLhs)) < std::atan2(start.x * lineRhs.y - start.y * lineRhs.x, start.dot(lineRhs));
        });
        glBegin(GL_LINE_LOOP);
        glColor4f(r, g, b, 1.);
        for (const auto & point : vertices) {
            glVertex3f(point.x, point.y, point.z);
        }
        glEnd();
        if (Annotation::singleton().annotationMode.testFlag(AnnotationMode::Mode_Paint)) { // fill brush with object color
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(r, g, b, .25);
            glVertex3f(center.x, center.y, center.z);
            for (const auto & point : vertices) {
                glVertex3f(point.x, point.y, point.z);
            }
            glVertex3f(vertices.front().x, vertices.front().y, vertices.front().z); // close triangle fan
            glEnd();
        }

    };
    const auto objColor = seg.colorOfSelectedObject();
    if (seg.brush.isInverse()) {
        drawCursor(1.f, 0.f, 0.f);
    } else {
        drawCursor(std::get<0>(objColor)/255., std::get<1>(objColor)/255., std::get<2>(objColor)/255.);
    }
    glPopMatrix();
}

void Viewport3D::renderArbitrarySlicePane(ViewportOrtho & vp, const RenderOptions & options) {
    // Used for calculation of slice pane length inside the 3d view
    const float dataPxX = vp.displayedIsoPx;
    const float dataPxY = vp.displayedIsoPx;

    for (std::size_t layerId{0}; layerId < Dataset::datasets.size(); ++layerId) {
        if (state->viewerState->layerRenderSettings[layerId].visible && (!Dataset::datasets[layerId].isOverlay() || options.drawOverlay)) {
            state->viewer->vpGenerateTexture(vp, layerId);// update texture before use
            auto & texture = vp.texture;
            texture.texHandle[layerId].bind();
            glBegin(GL_QUADS);
                glNormal3i(vp.n.x, vp.n.y, vp.n.z);
                glTexCoord2f(texture.texLUx, texture.texLUy);
                glVertex3f(-dataPxX * vp.v1.x - dataPxY * vp.v2.x,
                           -dataPxX * vp.v1.y - dataPxY * vp.v2.y,
                           -dataPxX * vp.v1.z - dataPxY * vp.v2.z);
                glTexCoord2f(texture.texRUx, texture.texRUy);
                glVertex3f( dataPxX * vp.v1.x - dataPxY * vp.v2.x,
                            dataPxX * vp.v1.y - dataPxY * vp.v2.y,
                            dataPxX * vp.v1.z - dataPxY * vp.v2.z);
                glTexCoord2f(texture.texRLx, texture.texRLy);
                glVertex3f( dataPxX * vp.v1.x + dataPxY * vp.v2.x,
                            dataPxX * vp.v1.y + dataPxY * vp.v2.y,
                            dataPxX * vp.v1.z + dataPxY * vp.v2.z);
                glTexCoord2f(texture.texLLx, texture.texLLy);
                glVertex3f(-dataPxX * vp.v1.x + dataPxY * vp.v2.x,
                           -dataPxX * vp.v1.y + dataPxY * vp.v2.y,
                           -dataPxX * vp.v1.z + dataPxY * vp.v2.z);
            glEnd();
            texture.texHandle[layerId].release();
        }
    }
}

boost::optional<nodeListElement &> ViewportBase::pickNode(int x, int y, int width) {
    const auto & nodes = pickNodes(x, y, width, width);
    if (nodes.size() != 0) {
        return *(*std::begin(nodes));
    }
    return boost::none;
}

hash_list<nodeListElement *> ViewportBase::pickNodes(int centerX, int centerY, int width, int height) {
    makeCurrent();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, this->width(), this->height());
    QOpenGLFramebufferObject fbo(this->width(), this->height(), QOpenGLFramebufferObject::CombinedDepthStencil);
    fbo.bind();
    auto pickingPass = [this, &fbo](auto flag){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderViewport(RenderOptions::nodePickingRenderOptions(flag));
        return fbo.toImage();
    };
    auto image24 = pickingPass(RenderOptions::SelectionPass::NodeID0_24Bits);
    auto image48 = pickingPass(RenderOptions::SelectionPass::NodeID24_48Bits);
    auto image64 = pickingPass(RenderOptions::SelectionPass::NodeID48_64Bits);
    fbo.release();
    glPopAttrib();

    hash_list<nodeListElement *> foundNodes;
    for (int d = 1; d <= std::max(height, width); d += 2) {
        const auto minx = std::max(0, centerX - std::min(d/2, width/2));
        const auto miny = std::max(0, centerY - std::min(d/2, height/2));
        const auto maxx = std::min(image24.width(), minx + std::min(d, width));
        const auto maxy = std::min(image24.height(), miny + std::min(d, height));
        for (int y = miny; y < maxy; y += 1)
        for (int x = minx; x < maxx; x += (y == miny || y == maxy - 1) ? 1 : d - 1) {
            const auto color24 = image24.pixelColor(x, y);
            const auto color48 = image48.pixelColor(x, y);
            const auto color64 = image64.pixelColor(x, y);
            std::array<int, 8> bytes{{color24.red(), color24.green(), color24.blue(), color48.red(), color48.green(), color48.blue(), color64.red(), color64.green()}};
            std::uint64_t name{};
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                name |= static_cast<std::uint64_t>(bytes[i]) << (8 * i);
            }
            nodeListElement * const foundNode = Skeletonizer::findNodeByNodeID(name - GLNames::NodeOffset);
            if (foundNode != nullptr) {
                foundNodes.emplace_back(foundNode);
            }
        }
    }
    return foundNodes;
}

std::pair<bool, bool> darkenOrHideTree(treeListElement & currentTree, const ViewportType vpType) {
    const auto * activeTree = state->skeletonState->activeTree;
    const auto * activeNode = state->skeletonState->activeNode;
    const auto * activeSynapse = (activeNode && activeNode->isSynapticNode) ? activeNode->correspondingSynapse :
                                 (activeTree && activeTree->isSynapticCleft) ? activeTree->correspondingSynapse :
                                                                               nullptr;
    const bool synapseBuilding = state->skeletonState->synapseState != Synapse::State::PreSynapse;
    const auto displayFlags = (vpType == VIEWPORT_SKELETON) ? state->viewerState->skeletonDisplayVP3D : state->viewerState->skeletonDisplayVPOrtho;
    const bool onlySelected = displayFlags.testFlag(TreeDisplay::OnlySelected);

    const bool darken = (synapseBuilding && currentTree.correspondingSynapse != &state->skeletonState->temporarySynapse)
            || (activeSynapse && activeSynapse->getCleft() != &currentTree);
    const bool hideSynapses = !darken && !synapseBuilding && !activeSynapse && currentTree.isSynapticCleft;
    const bool selectionFilter = onlySelected && !currentTree.selected;
    // hide synapse takes precedence over render flag for synapses.
    return std::make_pair(darken, selectionFilter || (!currentTree.render && (!currentTree.isSynapticCleft || (currentTree.isSynapticCleft && hideSynapses)) ));
}

template<typename Func>
void synapseLoop(Func func, const ViewportType vpType){
    for (auto & synapse : state->skeletonState->synapses) {
        const auto * activeTree = state->skeletonState->activeTree;
        const auto * activeNode = state->skeletonState->activeNode;
        const auto synapseCreated = synapse.getPostSynapse() != nullptr && synapse.getPreSynapse() != nullptr;
        const auto synapseSelected = synapse.getCleft() == activeTree || synapse.getPostSynapse() == activeNode || synapse.getPreSynapse() == activeNode;

        if (synapseCreated) {
            const auto synapseHidden = !synapse.getPreSynapse()->correspondingTree->render && !synapse.getPostSynapse()->correspondingTree->render;
            const auto displayFlags = (vpType == VIEWPORT_SKELETON) ? state->viewerState->skeletonDisplayVP3D : state->viewerState->skeletonDisplayVPOrtho;
            if (synapseHidden == false && (displayFlags.testFlag(TreeDisplay::OnlySelected) == false || synapseSelected)) {
                segmentListElement virtualSegment(*synapse.getPostSynapse(), *synapse.getPreSynapse());
                QColor color = Qt::black;
                if (synapseSelected == false) {
                    color.setAlpha(Synapse::darkenedAlpha);
                }
                func(synapse, virtualSegment, color);
            }
        }
    }
}

void generateSkeletonGeometry(GLBuffers & glBuffers, const RenderOptions &options, const ViewportType viewportType) {
    glBuffers.regenVertBuffer = false;
    glBuffers.lineVertBuffer.clear();
    glBuffers.pointVertBuffer.clear();
    glBuffers.colorPickingBuffer24.clear();
    glBuffers.colorPickingBuffer48.clear();
    glBuffers.colorPickingBuffer64.clear();

    auto arrayFromQColor = [](QColor color){
        return decltype(glBuffers.lineVertBuffer.colors)::value_type{{static_cast<std::uint8_t>(color.red()), static_cast<std::uint8_t>(color.green()), static_cast<std::uint8_t>(color.blue()), static_cast<std::uint8_t>(color.alpha())}};
    };

    auto addSegment = [arrayFromQColor, &glBuffers](const segmentListElement & segment, const QColor & color) {
        const auto isoBase = Dataset::current().scales[0].componentMul(segment.source.position);
        const auto isoTop = Dataset::current().scales[0].componentMul(segment.target.position);

        glBuffers.lineVertBuffer.emplace_back(isoBase, arrayFromQColor(color));
        glBuffers.lineVertBuffer.emplace_back(isoTop, arrayFromQColor(color));
    };

    auto addNode = [arrayFromQColor, options, &glBuffers](const nodeListElement & node) {
        auto color = state->viewer->getNodeColor(node);

        if (node.selected && options.highlightSelection) {// highlight selected nodes
            auto selectedNodeColor = QColor(Qt::green);
//            selectedNodeColor.setAlphaF(0.5f);// results in half-transparent nodes in low mode
            color = selectedNodeColor;
            glBuffers.pointVertBuffer.lastSelectedNode = node.nodeID;
        }

        const auto isoPos = Dataset::current().scales[0].componentMul(node.position);

        glBuffers.colorPickingBuffer24.emplace_back(arrayFromQColor(getPickingColor(node, RenderOptions::SelectionPass::NodeID0_24Bits)));
        glBuffers.colorPickingBuffer48.emplace_back(arrayFromQColor(getPickingColor(node, RenderOptions::SelectionPass::NodeID24_48Bits)));
        glBuffers.colorPickingBuffer64.emplace_back(arrayFromQColor(getPickingColor(node, RenderOptions::SelectionPass::NodeID48_64Bits)));
        glBuffers.pointVertBuffer.emplace_back(isoPos, arrayFromQColor(color));
        glBuffers.pointVertBuffer.colorBufferOffset[node.nodeID] = static_cast<unsigned int>(glBuffers.pointVertBuffer.vertices.size()-1);
    };

    for (auto & currentTree : Skeletonizer::singleton().skeletonState.trees) {
        // focus on synapses, darken rest of skeleton
        bool darken, hide;
        std::tie(darken, hide) = darkenOrHideTree(currentTree, viewportType);
        if (hide) {
            continue;
        }
        for (auto nodeIt = std::begin(currentTree.nodes); nodeIt != std::end(currentTree.nodes); ++nodeIt) {
            //This sets the current color for the segment rendering
            QColor currentColor = currentTree.color;
            if (state->viewerState->highlightActiveTree && currentTree.treeID == state->skeletonState->activeTree->treeID) {
                currentColor = Qt::red;
            }
            if (darken) {
                currentColor.setAlpha(Synapse::darkenedAlpha);
            }

            for (const auto & currentSegment : nodeIt->segments) {
                if (currentSegment.forward) {
                    continue;
                }
                addSegment(currentSegment, currentColor);
            }

            addNode(*nodeIt);
        }
    }

    synapseLoop([&addSegment](const auto &, const auto & virtualSegment, const auto & color){
        addSegment(virtualSegment, color);
    }, viewportType);

    const auto uploadVertexData = [](auto & buf, const auto & vertices){
        buf.destroy();
        buf.create();
        buf.bind();
        buf.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(vertices.front())));
        buf.release();
    };
    uploadVertexData(glBuffers.pointVertBuffer.color_buffer, glBuffers.pointVertBuffer.colors);
    uploadVertexData(glBuffers.pointVertBuffer.vertex_buffer, glBuffers.pointVertBuffer.vertices);

    uploadVertexData(glBuffers.lineVertBuffer.color_buffer, glBuffers.lineVertBuffer.colors);
    uploadVertexData(glBuffers.lineVertBuffer.vertex_buffer, glBuffers.lineVertBuffer.vertices);
}

/*
 * Fast and simplified tree rendering that uses frustum culling and
 * a heuristic level-of-detail implementation that exploits the implicit
 * sorting of the tree node list to avoid a depth first search for the compilation
 * of a spatial graph that is similar to the true skeleton, but without nodes /
 * vertices that would not be visible anyway. It uses large vertex batches for
 * line and point geometry (most data) drawn with vertex arrays, since the geometry is highly
 * dynamic (can change each frame). VBOs would make a lot of sense if we had a
 * smart spatial organization of the skeleton.
 * Ugly code, not nice to read, should be simplified...
 */
void ViewportBase::renderSkeleton(const RenderOptions &options) {
    if (!options.nodePicking && state->viewerState->lightOnOff) {
        // Configure light
        glEnable(GL_LIGHTING);
        GLfloat ambientLight[] = {0.5, 0.5, 0.5, 0.8};
        GLfloat diffuseLight[] = {1., 1., 1., 1.};
        GLfloat lightPos[] = {0, Dataset::current().boundary.y * Dataset::current().scales[0].y, 0, 1.};

        glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
        glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
        glEnable(GL_LIGHT0);

        GLfloat global_ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

        // Enable materials with automatic color tracking
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
        glShadeModel(GL_SMOOTH);
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_COLOR_MATERIAL);
    }

    //tdItem: test culling under different conditions!
    //if(viewportType == VIEWPORT_SKELETON) glEnable(GL_CULL_FACE);

    glPushMatrix();
    const auto displayFlag = (viewportType == VIEWPORT_SKELETON) ? state->viewerState->skeletonDisplayVP3D : state->viewerState->skeletonDisplayVPOrtho;
    auto & glBuffers = displayFlag.testFlag(TreeDisplay::OnlySelected) ? state->viewerState->selectedTreesBuffers : state->viewerState->AllTreesBuffers;
    if(glBuffers.regenVertBuffer) {
        generateSkeletonGeometry(glBuffers, options, viewportType);
    }
    if(!state->viewerState->onlyLinesAndPoints) {
        for (auto & currentTree : Skeletonizer::singleton().skeletonState.trees) {
            // focus on synapses, darken rest of skeleton
            bool darken, hide;
            std::tie(darken, hide) = darkenOrHideTree(currentTree, viewportType);
            if (hide) {
                continue;
            }
            nodeListElement * previousNode = nullptr;
            nodeListElement * lastRenderedNode = nullptr;
            float cumDistToLastRenderedNode = 0.f;

            for (auto nodeIt = std::begin(currentTree.nodes); nodeIt != std::end(currentTree.nodes); ++nodeIt) {
                /* We start with frustum culling:
                 * all nodes that are not in the current viewing frustum for the
                 * currently rendered viewports are discarded. This is very fast. */

                /* For frustum culling. These values should be stored, mem <-> cpu tradeoff  */

                /* Every node is tested based on a precomputed circumsphere
                that includes its segments. */

                if (!sphereInFrustum(Dataset::current().scales[0].componentMul(nodeIt->position), nodeIt->circRadius)) {
                    previousNode = lastRenderedNode = nullptr;
                    continue;
                }

                bool virtualSegRendered = false;
                bool nodeVisible = true;

                /* First test whether this node is actually connected to the next,
                i.e. whether the implicit sorting is not broken here. */
                bool allowHeuristic = false;
                if (std::next(nodeIt) != std::end(currentTree.nodes) && nodeIt->segments.size() <= 2) {
                    for (const auto & currentSegment : std::next(nodeIt)->segments) {
                        if (currentSegment.target == *nodeIt || currentSegment.source == *nodeIt) {
                            /* Connected, heuristic is allowed */
                            allowHeuristic = true;
                            break;
                        }
                    }
                }
                allowHeuristic = allowHeuristic && !options.nodePicking;
                if (previousNode != nullptr && allowHeuristic) {
                    for (auto & currentSegment : nodeIt->segments) {
                        //isBranchNode tells you only whether the node is on the branch point stack,
                        //not whether it is actually a node connected to more than two other nodes!
                        const bool mustBeRendered = nodeIt->getComment().isEmpty() == false || nodeIt->isBranchNode || nodeIt->segments.size() > 2 || nodeIt->radius * screenPxXPerDataPx > 5.f;
                        const bool cullingCandidate = currentSegment.target == *previousNode || (currentSegment.source == *previousNode && !mustBeRendered);
                        if (cullingCandidate) {
                            //Node is a candidate for LOD culling
                            //Do we really skip this node? Test cum dist. to last rendered node!
                            cumDistToLastRenderedNode += currentSegment.length * screenPxXPerDataPx;
                            if ((cumDistToLastRenderedNode <= state->viewerState->cumDistRenderThres) && options.enableLoddingAndLinesAndPoints) {
                                nodeVisible = false;
                            }
                            break;
                        }
                    }
                }

                if (nodeVisible) {
                    //This sets the current color for the segment rendering
                    QColor currentColor = currentTree.color;
                    if((currentTree.treeID == state->skeletonState->activeTree->treeID)
                        && (state->viewerState->highlightActiveTree)) {
                        currentColor = Qt::red;
                    }
                    if (darken) {
                        currentColor.setAlpha(Synapse::darkenedAlpha);
                    }

                    cumDistToLastRenderedNode = 0.f;

                    if (!options.nodePicking) {// don’t pick segments
                        if (previousNode != lastRenderedNode) {
                            virtualSegRendered = true;
                            // We need a "virtual" segment now
                            segmentListElement virtualSegment(*lastRenderedNode, *nodeIt, false);
                            renderSegment(virtualSegment, currentColor, options);
                        }
                        /* Second pass over segments needed... But only if node is actually rendered! */
                        for (const auto & currentSegment : nodeIt->segments) {
                            if (currentSegment.forward || (virtualSegRendered && (currentSegment.source == *previousNode || currentSegment.target == *previousNode))) {
                                continue;
                            }
                            renderSegment(currentSegment, currentColor, options);
                        }
                    }

                    renderNode(*nodeIt, options);
                    lastRenderedNode = &*nodeIt;
                }
                previousNode = &*nodeIt;
            }
        }

        /* Connect all synapses */
        if (!options.nodePicking) {
            synapseLoop([this, &options](const auto & synapse, const auto & virtualSegment, const auto & color){
                renderSegment(virtualSegment, color, options);

                auto post = synapse.getPostSynapse()->position;
                auto pre = synapse.getPreSynapse()->position;
                const auto offset = (post - pre)/10;
                Coordinate arrowbase = post - offset;

                renderCylinder(arrowbase, Skeletonizer::singleton().radius(*synapse.getPreSynapse()) * 3.0f
                    , synapse.getPostSynapse()->position
                    , Skeletonizer::singleton().radius(*synapse.getPostSynapse()) * state->viewerState->segRadiusToNodeRadius, color, options);
            }, viewportType);
        }
    }

    // lighting isn’t really applicable to lines and points
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    const auto alwaysLinesAndPoints = state->viewerState->cumDistRenderThres > 19.f && options.enableLoddingAndLinesAndPoints;
    // higher render qualities only use lines and points if node < smallestVisibleSize
    glLineWidth(alwaysLinesAndPoints ? lineSize(width()/displayedlengthInNmX) : smallestVisibleNodeSize());
    /* Render line geometry batch if it contains data and we don’t pick nodes */
    if (!options.nodePicking) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        /* draw all segments */
        glBuffers.lineVertBuffer.vertex_buffer.bind();
        glVertexPointer(3, GL_FLOAT, 0, nullptr);
        glBuffers.lineVertBuffer.vertex_buffer.release();

        glBuffers.lineVertBuffer.color_buffer.bind();
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);
        glBuffers.lineVertBuffer.color_buffer.release();

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(glBuffers.lineVertBuffer.vertices.size()));

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    glLineWidth(2.f);

    glPointSize(alwaysLinesAndPoints ? pointSize(width()/displayedlengthInNmX) : smallestVisibleNodeSize());
    /* Render point geometry batch if it contains data */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* draw all nodes */
    glBuffers.pointVertBuffer.vertex_buffer.bind();
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    glBuffers.pointVertBuffer.vertex_buffer.release();

    if(options.nodePicking) {
        if(options.selectionPass == RenderOptions::SelectionPass::NodeID0_24Bits) {
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, glBuffers.colorPickingBuffer24.data());
        } else if(options.selectionPass == RenderOptions::SelectionPass::NodeID24_48Bits) {
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, glBuffers.colorPickingBuffer48.data());
        } else if(options.selectionPass == RenderOptions::SelectionPass::NodeID48_64Bits) {
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, glBuffers.colorPickingBuffer64.data());
        }
    } else {
        glBuffers.pointVertBuffer.color_buffer.bind();
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, nullptr);
        glBuffers.pointVertBuffer.color_buffer.release();
    }

    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(glBuffers.pointVertBuffer.vertices.size()));

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPointSize(1.f);

    glPopMatrix(); // Restore modelview matrix
}

bool ViewportBase::updateFrustumClippingPlanes() {
   float   tmpFrustum[6][4];
   float   proj[16];
   float   modl[16];
   float   clip[16];

   /* Get the current PROJECTION matrix from OpenGL */
   glGetFloatv( GL_PROJECTION_MATRIX, proj );

   /* Get the current MODELVIEW matrix from OpenGL */
   glGetFloatv( GL_MODELVIEW_MATRIX, modl );

   /* Combine the two matrices (multiply projection by modelview) */
   clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
   clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
   clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
   clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

   clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
   clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
   clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
   clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

   clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
   clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
   clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
   clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

   clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
   clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
   clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
   clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

   /* Extract the numbers for the RIGHT plane */
   tmpFrustum[0][0] = clip[ 3] - clip[ 0];
   tmpFrustum[0][1] = clip[ 7] - clip[ 4];
   tmpFrustum[0][2] = clip[11] - clip[ 8];
   tmpFrustum[0][3] = clip[15] - clip[12];

   /* Normalize the result */
   float t = sqrt( tmpFrustum[0][0] * tmpFrustum[0][0] + tmpFrustum[0][1] * tmpFrustum[0][1] + tmpFrustum[0][2] * tmpFrustum[0][2] );
   tmpFrustum[0][0] /= t;
   tmpFrustum[0][1] /= t;
   tmpFrustum[0][2] /= t;
   tmpFrustum[0][3] /= t;

   /* Extract the numbers for the LEFT plane */
   tmpFrustum[1][0] = clip[ 3] + clip[ 0];
   tmpFrustum[1][1] = clip[ 7] + clip[ 4];
   tmpFrustum[1][2] = clip[11] + clip[ 8];
   tmpFrustum[1][3] = clip[15] + clip[12];

   /* Normalize the result */
   t = sqrt( tmpFrustum[1][0] * tmpFrustum[1][0] + tmpFrustum[1][1] * tmpFrustum[1][1] + tmpFrustum[1][2] * tmpFrustum[1][2] );
   tmpFrustum[1][0] /= t;
   tmpFrustum[1][1] /= t;
   tmpFrustum[1][2] /= t;
   tmpFrustum[1][3] /= t;

   /* Extract the BOTTOM plane */
   tmpFrustum[2][0] = clip[ 3] + clip[ 1];
   tmpFrustum[2][1] = clip[ 7] + clip[ 5];
   tmpFrustum[2][2] = clip[11] + clip[ 9];
   tmpFrustum[2][3] = clip[15] + clip[13];

   /* Normalize the result */
   t = sqrt( tmpFrustum[2][0] * tmpFrustum[2][0] + tmpFrustum[2][1] * tmpFrustum[2][1] + tmpFrustum[2][2] * tmpFrustum[2][2] );
   tmpFrustum[2][0] /= t;
   tmpFrustum[2][1] /= t;
   tmpFrustum[2][2] /= t;
   tmpFrustum[2][3] /= t;

   /* Extract the TOP plane */
   tmpFrustum[3][0] = clip[ 3] - clip[ 1];
   tmpFrustum[3][1] = clip[ 7] - clip[ 5];
   tmpFrustum[3][2] = clip[11] - clip[ 9];
   tmpFrustum[3][3] = clip[15] - clip[13];

   /* Normalize the result */
   t = sqrt( tmpFrustum[3][0] * tmpFrustum[3][0] + tmpFrustum[3][1] * tmpFrustum[3][1] + tmpFrustum[3][2] * tmpFrustum[3][2] );
   tmpFrustum[3][0] /= t;
   tmpFrustum[3][1] /= t;
   tmpFrustum[3][2] /= t;
   tmpFrustum[3][3] /= t;

   /* Extract the FAR plane */
   tmpFrustum[4][0] = clip[ 3] - clip[ 2];
   tmpFrustum[4][1] = clip[ 7] - clip[ 6];
   tmpFrustum[4][2] = clip[11] - clip[10];
   tmpFrustum[4][3] = clip[15] - clip[14];

   /* Normalize the result */
   t = sqrt( tmpFrustum[4][0] * tmpFrustum[4][0] + tmpFrustum[4][1] * tmpFrustum[4][1] + tmpFrustum[4][2] * tmpFrustum[4][2] );
   tmpFrustum[4][0] /= t;
   tmpFrustum[4][1] /= t;
   tmpFrustum[4][2] /= t;
   tmpFrustum[4][3] /= t;

   /* Extract the NEAR plane */
   tmpFrustum[5][0] = clip[ 3] + clip[ 2];
   tmpFrustum[5][1] = clip[ 7] + clip[ 6];
   tmpFrustum[5][2] = clip[11] + clip[10];
   tmpFrustum[5][3] = clip[15] + clip[14];

   /* Normalize the result */
   t = sqrt( tmpFrustum[5][0] * tmpFrustum[5][0] + tmpFrustum[5][1] * tmpFrustum[5][1] + tmpFrustum[5][2] * tmpFrustum[5][2] );
   tmpFrustum[5][0] /= t;
   tmpFrustum[5][1] /= t;
   tmpFrustum[5][2] /= t;
   tmpFrustum[5][3] /= t;

   memcpy(frustum, tmpFrustum, sizeof(tmpFrustum));
    return true;
}

/* modified public domain code from: http://www.crownandcutlass.com/features/technicaldetails/frustum.html */
bool ViewportBase::sphereInFrustum(floatCoordinate pos, float radius) {
    for(int p = 0; p < 6; ++p) {
        if(frustum[p][0] * pos.x + frustum[p][1] * pos.y + frustum[p][2] * pos.z + frustum[p][3] <= -radius) {
           return false;
        }
    }
    return true;
}

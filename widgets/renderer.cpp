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
#include "widgets/viewport.h"

#include "mesh.h"
#include "profiler.h"
#include "segmentation/cubeloader.h"
#include "segmentation/segmentation.h"
#include "session.h"
#include "skeleton/node.h"
#include "skeleton/skeletonizer.h"
#include "skeleton/tree.h"
#include "viewer.h"

#include <QMatrix4x4>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QVector3D>

#ifdef Q_OS_MAC
    #include <glu.h>
#endif
#ifdef Q_OS_LINUX
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif
#ifdef Q_OS_WIN32
    #include <GL/glu.h>
#endif

#include <boost/math/constants/constants.hpp>

#include <cmath>

#define ROTATIONSTATEXY    0
#define ROTATIONSTATEXZ    1
#define ROTATIONSTATEYZ    2
#define ROTATIONSTATERESET 3

bool ViewportBase::initMesh(mesh & toInit, uint initialSize) {
    toInit.vertices = (floatCoordinate *)malloc(initialSize * sizeof(floatCoordinate));
    toInit.normals = (floatCoordinate *)malloc(initialSize * sizeof(floatCoordinate));
    toInit.colors = (color4F *)malloc(initialSize * sizeof(color4F));

    toInit.vertsBuffSize = initialSize;
    toInit.normsBuffSize = initialSize;
    toInit.colsBuffSize = initialSize;

    toInit.vertsIndex = 0;
    toInit.normsIndex = 0;
    toInit.colsIndex = 0;
    return true;
}

bool ViewportBase::doubleMeshCapacity(mesh & toDouble) {
    return resizemeshCapacity(toDouble, 2);
}

bool ViewportBase::resizemeshCapacity(mesh & toResize, uint n) {
    toResize.vertices = (floatCoordinate *)realloc(toResize.vertices, n * toResize.vertsBuffSize * sizeof(floatCoordinate));
    toResize.normals = (floatCoordinate *)realloc(toResize.normals, n * toResize.normsBuffSize * sizeof(floatCoordinate));
    toResize.colors = (color4F *)realloc(toResize.colors, n * toResize.colsBuffSize * sizeof(color4F));

    toResize.vertsBuffSize = n * toResize.vertsBuffSize;
    toResize.normsBuffSize = n * toResize.normsBuffSize;
    toResize.colsBuffSize = n * toResize.colsBuffSize;

    return true;
}

bool setRotationState(uint setTo) {
    if (setTo == ROTATIONSTATERESET){
        state->skeletonState->rotationState[0] = 0.866025;
        state->skeletonState->rotationState[1] = 0.286788;
        state->skeletonState->rotationState[2] = 0.409576;
        state->skeletonState->rotationState[3] = 0.0;
        state->skeletonState->rotationState[4] = -0.5;
        state->skeletonState->rotationState[5] = 0.496732;
        state->skeletonState->rotationState[6] = 0.709407;
        state->skeletonState->rotationState[7] = 0.0;
        state->skeletonState->rotationState[8] = 0.0;
        state->skeletonState->rotationState[9] = 0.819152;
        state->skeletonState->rotationState[10] = -0.573576;
        state->skeletonState->rotationState[11] = 0.0;
        state->skeletonState->rotationState[12] = 0.0;
        state->skeletonState->rotationState[13] = 0.0;
        state->skeletonState->rotationState[14] = 0.0;
        state->skeletonState->rotationState[15] = 1.0;
    }
    if (setTo == ROTATIONSTATEXY){//x @ 0°
        state->skeletonState->rotationState[0] = 1.0;//1
        state->skeletonState->rotationState[1] = 0.0;
        state->skeletonState->rotationState[2] = 0.0;
        state->skeletonState->rotationState[3] = 0.0;
        state->skeletonState->rotationState[4] = 0.0;
        state->skeletonState->rotationState[5] = 1.0;//cos
        state->skeletonState->rotationState[6] = 0.0;//-sin
        state->skeletonState->rotationState[7] = 0.0;
        state->skeletonState->rotationState[8] = 0.0;
        state->skeletonState->rotationState[9] = 0.0;//sin
        state->skeletonState->rotationState[10] = 1.0;//cos
        state->skeletonState->rotationState[11] = 0.0;
        state->skeletonState->rotationState[12] = 0.0;
        state->skeletonState->rotationState[13] = 0.0;
        state->skeletonState->rotationState[14] = 0.0;
        state->skeletonState->rotationState[15] = 1.0;//1
    }
    if (setTo == ROTATIONSTATEXZ){//x @ 90°
        state->skeletonState->rotationState[0] = 1.0;//1
        state->skeletonState->rotationState[1] = 0.0;
        state->skeletonState->rotationState[2] = 0.0;
        state->skeletonState->rotationState[3] = 0.0;
        state->skeletonState->rotationState[4] = 0.0;
        state->skeletonState->rotationState[5] = 0.0;//cos
        state->skeletonState->rotationState[6] = -1.0;//-sin
        state->skeletonState->rotationState[7] = 0.0;
        state->skeletonState->rotationState[8] = 0.0;
        state->skeletonState->rotationState[9] = 1.0;//sin
        state->skeletonState->rotationState[10] = 0.0;//cos
        state->skeletonState->rotationState[11] = 0.0;
        state->skeletonState->rotationState[12] = 0.0;
        state->skeletonState->rotationState[13] = 0.0;
        state->skeletonState->rotationState[14] = 0.0;
        state->skeletonState->rotationState[15] = 1.0;//1
    }
    if (setTo == ROTATIONSTATEYZ){//y @ -90°
        state->skeletonState->rotationState[0] = 0.0;//cos
        state->skeletonState->rotationState[1] = 0.0;
        state->skeletonState->rotationState[2] = -1.0;//sin
        state->skeletonState->rotationState[3] = 0.0;
        state->skeletonState->rotationState[4] = 0.0;
        state->skeletonState->rotationState[5] = 1.0;//1
        state->skeletonState->rotationState[6] = 0.0;
        state->skeletonState->rotationState[7] = 0.0;
        state->skeletonState->rotationState[8] = 1.0;//-sin
        state->skeletonState->rotationState[9] = 0.0;
        state->skeletonState->rotationState[10] = 0.0;//cos
        state->skeletonState->rotationState[11] = 0.0;
        state->skeletonState->rotationState[12] = 0.0;
        state->skeletonState->rotationState[13] = 0.0;
        state->skeletonState->rotationState[14] = 0.0;
        state->skeletonState->rotationState[15] = 1.0;//1
    }
    return true;
}


uint ViewportBase::renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color) {
    float currentAngle = 0.;
    floatCoordinate segDirection, tempVec, tempVec2;
    GLUquadricObj *gluCylObj = NULL;

    if(((screenPxXPerDataPx * baseRadius < 1.f) && (screenPxXPerDataPx * topRadius < 1.f)) || (state->viewerState->cumDistRenderThres > 19.f)) {

        if(state->skeletonState->lineVertBuffer.vertsBuffSize < state->skeletonState->lineVertBuffer.vertsIndex + 2)
            doubleMeshCapacity(state->skeletonState->lineVertBuffer);

        state->skeletonState->lineVertBuffer.vertices[state->skeletonState->lineVertBuffer.vertsIndex] = Coordinate{base->x, base->y, base->z};
        state->skeletonState->lineVertBuffer.vertices[state->skeletonState->lineVertBuffer.vertsIndex + 1] = Coordinate{top->x, top->y, top->z};

        state->skeletonState->lineVertBuffer.colors[state->skeletonState->lineVertBuffer.vertsIndex] = color;
        state->skeletonState->lineVertBuffer.colors[state->skeletonState->lineVertBuffer.vertsIndex + 1] = color;

        state->skeletonState->lineVertBuffer.vertsIndex += 2;

    } else {
        GLfloat a[] = {color.r, color.g, color.b, color.a};

        glColor4fv(a);
        //glColor4fv(&color);

        glPushMatrix();
        gluCylObj = gluNewQuadric();
        gluQuadricNormals(gluCylObj, GLU_SMOOTH);
        gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);

        glTranslatef((float)base->x, (float)base->y, (float)base->z);

        //Some calculations for the correct direction of the cylinder.
        tempVec = {0., 0., 1.};
        segDirection = {(float)(top->x - base->x), (float)(top->y - base->y), (float)(top->z - base->z)};

        //temVec2 defines the rotation axis
        tempVec2 = crossProduct(tempVec, segDirection);
        currentAngle = radToDeg(vectorAngle(tempVec, segDirection));

        //some gl implementations have problems with the params occuring for
        //segs in straight directions. we need a fix here.
        glRotatef(currentAngle, tempVec2.x, tempVec2.y, tempVec2.z);

        //tdItem use screenPxXPerDataPx for proper LOD
        //the same values have to be used in rendersegplaneintersections to avoid ugly graphics
        if((baseRadius > 100.f) || topRadius > 100.f) {
            gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(segDirection), 10, 1);
        }
        else if((baseRadius > 15.f) || topRadius > 15.f) {
            gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(segDirection), 6, 1);
        }
        else {
            gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(segDirection), 3, 1);
        }

        gluDeleteQuadric(gluCylObj);
        glPopMatrix();
    }
    return true;
}

uint ViewportBase::renderSphere(const Coordinate & pos, const float & radius, const color4F & color) {
    GLUquadricObj *gluSphereObj = NULL;

    /* Render only a point if the sphere wouldn't be visible anyway */
    if(((screenPxXPerDataPx * radius > 0.0f) && (screenPxXPerDataPx * radius < 2.0f)) || (state->viewerState->cumDistRenderThres > 19.f)) {
        /* This is cumbersome, but SELECT mode cannot be used with glDrawArray.
        Color buffer picking brings its own issues on the other hand, so we
        stick with SELECT mode for the time being. */
        if(state->viewerState->selectModeFlag) {
            glPointSize(radius * 2.);
            glBegin(GL_POINTS);
                glVertex3f((float)pos.x, (float)pos.y, (float)pos.z);
            glEnd();
            glPointSize(1.);
        }
        else {
            if(state->skeletonState->pointVertBuffer.vertsBuffSize < state->skeletonState->pointVertBuffer.vertsIndex + 2)
                doubleMeshCapacity(state->skeletonState->pointVertBuffer);

            state->skeletonState->pointVertBuffer.vertices[state->skeletonState->pointVertBuffer.vertsIndex] = Coordinate{pos.x, pos.y, pos.z};
            state->skeletonState->pointVertBuffer.colors[state->skeletonState->pointVertBuffer.vertsIndex] = color;
            state->skeletonState->pointVertBuffer.vertsIndex++;
        }
    }
    else {
        GLfloat tmp[] = {color.r, color.g, color.b, color.a};
        glColor4fv(tmp);
        glPushMatrix();
        glTranslatef((float)pos.x, (float)pos.y, (float)pos.z);
        glScalef(1.f, 1.f, state->viewerState->voxelXYtoZRatio);
        gluSphereObj = gluNewQuadric();
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
        //glScalef(1.f, 1.f, 1.f/state->viewerState->voxelXYtoZRatio);
        gluDeleteQuadric(gluSphereObj);
        glPopMatrix();
    }

    return true;
}

void ViewportBase::renderSegment(const segmentListElement & segment, const color4F & color) {
    renderCylinder(&(segment.source->position), Skeletonizer::singleton().segmentSizeAt(*segment.source) * state->viewerState->segRadiusToNodeRadius,
        &(segment.target->position), Skeletonizer::singleton().segmentSizeAt(*segment.target) * state->viewerState->segRadiusToNodeRadius, color);
}

void ViewportOrtho::renderSegment(const segmentListElement & segment, const color4F & color) {
    ViewportBase::renderSegment(segment, color);
    if (state->viewerState->showIntersections) {
        renderSegPlaneIntersection(segment);
    }
}

void ViewportBase::renderNode(const nodeListElement & node, const RenderOptions & options) {
    auto color = state->viewer->getNodeColor(node);
    const float radius = Skeletonizer::singleton().radius(node);

    renderSphere(node.position, radius, color);

    if(node.selected && options.highlightSelection) { // highlight selected nodes
        renderSphere(node.position, radius * 2, {0.f, 1.f, 0.f, 0.5f});
    }
    // Highlight active node with a halo
    if(state->skeletonState->activeNode == &node && options.highlightActiveNode) {
        // Color gets changes in case there is a comment & conditional comment highlighting
        color4F haloColor = state->viewer->getNodeColor(node);
        haloColor.a = 0.2f;
        renderSphere(node.position, Skeletonizer::singleton().radius(node) * 1.5, haloColor);
    }
}

void ViewportOrtho::renderNode(const nodeListElement & node, const RenderOptions & options) {
    ViewportBase::renderNode(node, options);
    if(1.5 <  Skeletonizer::singleton().radius(node)) { // draw node center to make large nodes visible and clickable in ortho vps
        color4F color = state->viewer->getNodeColor(node);
        renderSphere(node.position, 1.5, color);
    }
    // Render the node description
    glColor4f(0.f, 0.f, 0.f, 1.f);
    auto nodeID = (state->viewerState->showNodeIDs || state->skeletonState->activeNode == &node)? QString::number(node.nodeID) : "";
    const auto comment = (ViewportOrtho::showNodeComments && node.comment)? QString(":%1").arg(node.comment->content) : "";
    if(nodeID.isEmpty() == false || comment.isEmpty() == false) {
        renderText(node.position, nodeID.append(comment));
    }
}

void Viewport3D::renderNode(const nodeListElement & node, const RenderOptions & options) {
    ViewportBase::renderNode(node, options);
    // Render the node description
    if(state->viewerState->showNodeIDs || state->skeletonState->activeNode == &node) {
        glColor4f(0.f, 0.f, 0.f, 1.f);
        renderText(node.position, QString::number(node.nodeID));
    }
}

static void backup_gl_state() {
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
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

void ViewportBase::renderText(const Coordinate & pos, const QString & str, const int fontSize, bool centered) {
    GLdouble x, y, z, model[16], projection[16];
    GLint gl_viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, &model[0]);
    glGetDoublev(GL_PROJECTION_MATRIX, &projection[0]);
    glGetIntegerv(GL_VIEWPORT, gl_viewport);
    //retrieve 2d screen position from coordinate
    backup_gl_state();
    QOpenGLPaintDevice paintDevice(gl_viewport[2], gl_viewport[3]);//create paint device from viewport size and current context
    QPainter painter(&paintDevice);
    painter.setFont(QFont(painter.font().family(), fontSize * devicePixelRatio()));
    gluProject(pos.x, pos.y - 3, pos.z, &model[0], &projection[0], &gl_viewport[0], &x, &y, &z);
    painter.setPen(Qt::black);
    painter.drawText(centered ? x - QFontMetrics(painter.font()).width(str)/2. : x, gl_viewport[3] - y, str);//inverse y coordinate, extract height from gl viewport
    painter.end();//would otherwise fiddle with the gl state in the dtor
    restore_gl_state();
}


uint ViewportOrtho::renderSegPlaneIntersection(const segmentListElement & segment) {
    float p[2][3], a, currentAngle, length, radius, distSourceInter, sSr_local, sTr_local;
    int i, distToCurrPos;
    floatCoordinate tempVec2, tempVec, tempVec3, segDir, intPoint, sTp_local, sSp_local;
    GLUquadricObj *gluCylObj = NULL;

    sSp_local.x = (float)segment.source->position.x;
    sSp_local.y = (float)segment.source->position.y;
    sSp_local.z = (float)segment.source->position.z;

    sTp_local.x = (float)segment.target->position.x;
    sTp_local.y = (float)segment.target->position.y;
    sTp_local.z = (float)segment.target->position.z;

    sSr_local = (float)segment.source->radius;
    sTr_local = (float)segment.target->radius;

    //n contains the normal vectors of the 3 orthogonal planes
    float n[3][3] = {{1.,0.,0.},
                    {0.,1.,0.},
                    {0.,0.,1.}};

    distToCurrPos = ((state->M / 2) + 1) + 1 * state->cubeEdgeLength;

    //Check if there is an intersection between the given segment and one
    //of the slice planes.
    p[0][0] = sSp_local.x - (float)state->viewerState->currentPosition.x;
    p[0][1] = sSp_local.y - (float)state->viewerState->currentPosition.y;
    p[0][2] = sSp_local.z - (float)state->viewerState->currentPosition.z;

    p[1][0] = sTp_local.x - (float)state->viewerState->currentPosition.x;
    p[1][1] = sTp_local.y - (float)state->viewerState->currentPosition.y;
    p[1][2] = sTp_local.z - (float)state->viewerState->currentPosition.z;


    //i represents the current orthogonal plane
    for(i = 0; i<=2; i++) {
        //There is an intersection and the segment doesn't lie in the plane
        if(sgn(p[0][i])*sgn(p[1][i]) == -1) {
            //Calculate intersection point
            segDir.x = sTp_local.x - sSp_local.x;
            segDir.y = sTp_local.y - sSp_local.y;
            segDir.z = sTp_local.z - sSp_local.z;

            //a is the scaling factor for the straight line equation: g:=segDir*a+v0
            a = (n[i][0] * (((float)state->viewerState->currentPosition.x - sSp_local.x))
                    + n[i][1] * (((float)state->viewerState->currentPosition.y - sSp_local.y))
                    + n[i][2] * (((float)state->viewerState->currentPosition.z - sSp_local.z)))
                / (segDir.x*n[i][0] + segDir.y*n[i][1] + segDir.z*n[i][2]);

            tempVec3.x = segDir.x * a;
            tempVec3.y = segDir.y * a;
            tempVec3.z = segDir.z * a;

            intPoint.x = tempVec3.x + sSp_local.x;
            intPoint.y = tempVec3.y + sSp_local.y;
            intPoint.z = tempVec3.z + sSp_local.z;

            //Check wether the intersection point lies outside the current zoom cube
            if(abs((int)intPoint.x - state->viewerState->currentPosition.x) <= distToCurrPos
                && abs((int)intPoint.y - state->viewerState->currentPosition.y) <= distToCurrPos
                && abs((int)intPoint.z - state->viewerState->currentPosition.z) <= distToCurrPos) {

                //Render a cylinder to highlight the intersection
                glPushMatrix();
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);

                length = euclidicNorm(segDir);
                distSourceInter = euclidicNorm(tempVec3);

                if(sSr_local < sTr_local)
                    radius = sTr_local + sSr_local * (1. - distSourceInter / length);
                else if(sSr_local == sTr_local)
                    radius = sSr_local;
                else
                    radius = sSr_local - (sSr_local - sTr_local) * distSourceInter / length;

                segDir = {segDir.x / length, segDir.y / length, segDir.z / length};

                glTranslatef(intPoint.x - 0.75 * segDir.x, intPoint.y - 0.75 * segDir.y, intPoint.z - 0.75 * segDir.z);
                //glTranslatef(intPoint.x, intPoint.y, intPoint.z);

                //Some calculations for the correct direction of the cylinder.
                tempVec = {0., 0., 1.};
                //temVec2 defines the rotation axis
                tempVec2 = crossProduct(tempVec, segDir);
                currentAngle = radToDeg(vectorAngle(tempVec, segDir));
                glRotatef(currentAngle, tempVec2.x, tempVec2.y, tempVec2.z);

                glColor4f(0.,0.,0.,1.);

                if(state->viewerState->overrideNodeRadiusBool)
                    gluCylinder(gluCylObj,
                        state->viewerState->overrideNodeRadiusVal * state->viewerState->segRadiusToNodeRadius*1.2,
                        state->viewerState->overrideNodeRadiusVal * state->viewerState->segRadiusToNodeRadius*1.2,
                        1.5, 3, 1);

                else gluCylinder(gluCylObj,
                        radius * state->viewerState->segRadiusToNodeRadius*1.2,
                        radius * state->viewerState->segRadiusToNodeRadius*1.2,
                        1.5, 3, 1);

                gluDeleteQuadric(gluCylObj);
                glPopMatrix();
            }

        }
    }
    return true;
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

    switch(viewportType) {
        case VIEWPORT_XY:
            glColor4f(0.7, 0., 0., 1.);
            break;
        case VIEWPORT_XZ:
            glColor4f(0., 0.7, 0., 1.);
            break;
        case VIEWPORT_YZ:
            glColor4f(0., 0., 0.7, 1.);
            break;
        case VIEWPORT_SKELETON:
            glColor4f(0., 0., 0., 1.);
            break;
        case VIEWPORT_ARBITRARY:
            glColor4f(n.z, n.y, n.x, 1.);
        break;
    }
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

    if(viewportType == state->viewerState->highlightVp) {
        // Draw an orange border to highlight the viewport.
        glColor4f(1., 0.3, 0., 1.);
        glBegin(GL_LINE_LOOP);
            glVertex3f(3, 3, -1);
            glVertex3f(edgeLength - 3, 3, -1);
            glVertex3f(edgeLength - 3, edgeLength - 3, -1);
            glVertex3f(3, edgeLength - 3, -1);
        glEnd();
    }
    glLineWidth(1.);

    // render node selection box
    if (state->viewerState->nodeSelectSquareVpId == static_cast<int>(id)) {
        Coordinate leftUpper = state->viewerState->nodeSelectionSquare.first;
        Coordinate rightLower = state->viewerState->nodeSelectionSquare.second;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
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
        glDisable(GL_BLEND);
    }
    if(state->viewerState->showScalebar) {
        if(id == VIEWPORT_SKELETON && Segmentation::singleton().volume_render_toggle) {
            return;
        }
        renderScaleBar();
    }
}

void ViewportBase::renderScaleBar(const int fontSize) {
    const auto vp_edgelen_um = 0.001 * displayedlengthInNmX;
    auto rounded_scalebar_len_um = std::round(vp_edgelen_um/3 * 2) / 2; // round to next 0.5
    auto sizeLabel = QString("%1 µm").arg(rounded_scalebar_len_um);
    auto divisor = vp_edgelen_um / rounded_scalebar_len_um; // for scalebar size in pixels

    if(rounded_scalebar_len_um == 0) {
        const auto rounded_scalebar_len_nm = std::round(displayedlengthInNmX/3/5)*5; // switch to nanometers rounded to next multiple of 5
        sizeLabel = QString("%1 nm").arg(rounded_scalebar_len_nm);
        divisor = displayedlengthInNmX/rounded_scalebar_len_nm;
    }
    int min_x = 0.02 * edgeLength, max_x = min_x + edgeLength / divisor, min_y = edgeLength - min_x - 2, max_y = min_y + 2, z = -1;
    glColor3f(0., 0., 0.);
    glBegin(GL_POLYGON);
    glVertex3d(min_x, min_y, z);
    glVertex3d(max_x, min_y, z);
    glVertex3d(max_x, max_y, z);
    glVertex3d(min_x, max_y, z);
    glEnd();

    renderText(Coordinate(min_x + edgeLength / divisor / 2, min_y, z), sizeLabel, fontSize, true);
}

void ViewportOrtho::renderViewportFast() {
    if (state->viewer->layers.empty()) {
        return;
    }
    static QElapsedTimer time;
//    qDebug() << time.restart();

    QOpenGLTimeMonitor times;
    times.setSampleCount(3);
    times.create();
    times.recordSample();

    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const bool xy = viewportType == VIEWPORT_XY, xz = viewportType == VIEWPORT_XZ, zy = viewportType == VIEWPORT_YZ;
    const auto gpucubeedge = state->viewer->gpucubeedge;
    const auto supercubeedge = state->M * state->cubeEdgeLength / gpucubeedge - (state->cubeEdgeLength / gpucubeedge - 1);
//    QVector3D offset;
    const auto cpos = state->viewerState->currentPosition;
    const int xxx = cpos.x % gpucubeedge;
    const int yyy = cpos.y % gpucubeedge;
    const int zzz = cpos.z % gpucubeedge;
    const float frame = xy ? zzz : xz ? yyy : xxx;
//    QVector3D deviation(zy ? 0 : xxx, xz ? 0 : -yyy - zy * gpucubeedge * state->scale.z / state->scale.y, xy ? 0 : -zzz - xz * gpucubeedge * state->scale.z / state->scale.x);
    QVector3D deviation(zy ? 0 : xxx, xz ? 0 : -yyy, xy ? 0 : -zzz);
//    qDebug() << deviation;

    std::vector<std::array<GLfloat, 3>> triangleVertices;//flipped y
    triangleVertices.push_back({{0.0f, 1.0f, 0.0f}});
    triangleVertices.push_back({{0.0f, 0.0f, 0.0f}});
    triangleVertices.push_back({{1.0f, 0.0f, 0.0f}});
    triangleVertices.push_back({{1.0f, 1.0f, 0.0f}});
    std::vector<std::array<GLfloat, 3>> textureVertices;
    for (float z = 0.0f; z < (xy ? 1 : supercubeedge); ++z)
    for (float y = 0.0f; y < (xz ? 1 : supercubeedge); ++y)
    for (float x = 0.0f; x < (zy ? 1 : supercubeedge); ++x) {
        const float cubeedgef = gpucubeedge;
        const auto depthOffset = frame;//xy ? deviation.z() : xz ? deviation.y() : deviation.x();
        const auto starttexR = (0.5f + depthOffset) / cubeedgef;
        const auto endtexR = (0.5f + depthOffset) / cubeedgef;

        if (xy) {
            textureVertices.push_back({{0.0f, 1.0f, starttexR}});
            textureVertices.push_back({{0.0f, 0.0f, starttexR}});
            textureVertices.push_back({{1.0f, 0.0f, endtexR}});
            textureVertices.push_back({{1.0f, 1.0f, endtexR}});
        } else if (xz) {
            textureVertices.push_back({{0.0f, starttexR, 1.0f}});
            textureVertices.push_back({{0.0f, starttexR, 0.0f}});
            textureVertices.push_back({{1.0f, endtexR, 0.0f}});
            textureVertices.push_back({{1.0f, endtexR, 1.0f}});
        } else if (zy) {
            textureVertices.push_back({{starttexR, 1.0f, 0.0f}});
            textureVertices.push_back({{starttexR, 0.0f, 0.0f}});
            textureVertices.push_back({{endtexR, 0.0f, 1.0f}});
            textureVertices.push_back({{endtexR, 1.0f, 1.0f}});
        }
    }

    QMatrix4x4 modelMatrix; //identity
    QMatrix4x4 viewMatrix; //identity
    QMatrix4x4 projectionMatrix;
    const float width = 1.0f * this->width();
    const float height = 1.0f * this->height();
    projectionMatrix.ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);//origin top left

//    viewMatrix.scale(zy ? state->scale.z / state->scale.y : 1, xz ? state->scale.z / state->scale.x : 1, 1);
    viewMatrix.scale(width / (gpucubeedge * supercubeedge - state->cubeEdgeLength), height / (gpucubeedge * supercubeedge - state->cubeEdgeLength));
    if (xz) {
        viewMatrix.rotate(-90.0f, QVector3D(1.0f, 0.0f, 0.0f));
    } else if (zy) {
        viewMatrix.rotate(-90.0f, QVector3D(0.0f, -1.0f, 0.0f));
    }
    viewMatrix.translate(deviation / QVector3D{-1.0f, 1.0f, 1.0f});

//    viewMatrix.translate(0, 0, -zy * gpucubeedge * supercubeedge * 0.5 / state->scale.z / state->scale.y -xz * gpucubeedge * supercubeedge * 0.5 / state->scale.z / state->scale.x);

    // raw data shader
    raw_data_shader.bind();
    int vertexLocation = raw_data_shader.attributeLocation("vertex");
    int texLocation = raw_data_shader.attributeLocation("texCoordVertex");
    raw_data_shader.enableAttributeArray(vertexLocation);
    raw_data_shader.enableAttributeArray(texLocation);
    raw_data_shader.setAttributeArray(vertexLocation, triangleVertices.data()->data(), 3);
    raw_data_shader.setAttributeArray(texLocation, textureVertices.data()->data(), 3);
    raw_data_shader.setUniformValue("model_matrix", modelMatrix);
    raw_data_shader.setUniformValue("view_matrix", viewMatrix);
    raw_data_shader.setUniformValue("projection_matrix", projectionMatrix);
    raw_data_shader.setUniformValue("texture", 0);

    // overlay data shader
    overlay_data_shader.bind();
    int overtexLocation = overlay_data_shader.attributeLocation("vertex");
    int otexLocation = overlay_data_shader.attributeLocation("texCoordVertex");
    overlay_data_shader.enableAttributeArray(overtexLocation);
    overlay_data_shader.enableAttributeArray(otexLocation);
    overlay_data_shader.setAttributeArray(overtexLocation, triangleVertices.data()->data(), 3);
    overlay_data_shader.setAttributeArray(otexLocation, textureVertices.data()->data(), 3);
    overlay_data_shader.setUniformValue("model_matrix", modelMatrix);
    overlay_data_shader.setUniformValue("view_matrix", viewMatrix);
    overlay_data_shader.setUniformValue("projection_matrix", projectionMatrix);
    overlay_data_shader.setUniformValue("indexTexture", 0);
    overlay_data_shader.setUniformValue("textureLUT", 1);
    overlay_data_shader.setUniformValue("factor", static_cast<float>(std::numeric_limits<gpu_lut_cube::gpu_index>::max()));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_3D);

    for (auto & layer : state->viewer->layers) {
        if (layer.enabled && layer.opacity >= 0.0f) {
            if (layer.isOverlayData) {
                overlay_data_shader.bind();
                overlay_data_shader.setUniformValue("textureOpacity", Segmentation::singleton().alpha / 256.0f);
            } else {
                raw_data_shader.bind();
                raw_data_shader.setUniformValue("textureOpacity", layer.opacity);
            }

//            const float offsetx = 82;
//            const float offsety = 82;
//            const float offsetz = 42;
            const float halfsc = (supercubeedge * gpucubeedge - state->cubeEdgeLength) * 0.5f / gpucubeedge;
            const float offsetx = state->viewerState->currentPosition.x / gpucubeedge - halfsc * !zy;
            const float offsety = state->viewerState->currentPosition.y / gpucubeedge - halfsc * !xz;
            const float offsetz = state->viewerState->currentPosition.z / gpucubeedge - halfsc * !xy;
//            qDebug() << offsetx << offsety << offsetz << gpucubeedge << supercubeedge;
            const float startx = 0 * state->viewerState->currentPosition.x / gpucubeedge;
            const float starty = 0 * state->viewerState->currentPosition.y / gpucubeedge;
            const float startz = 0 * state->viewerState->currentPosition.z / gpucubeedge;
            const float endx = startx + (zy ? 1 : supercubeedge);
            const float endy = starty + (xz ? 1 : supercubeedge);
            const float endz = startz + (xy ? 1 : supercubeedge);
            for (float z = startz; z < endz; ++z)
            for (float y = starty; y < endy; ++y)
            for (float x = startx; x < endx; ++x) {
                auto pos = QVector3D(offsetx + x, offsety + y, offsetz + z);
                auto it = layer.textures.find(pos);
                auto & ptr = it != std::end(layer.textures) ? *it->second : *layer.bogusCube;
                modelMatrix.setToIdentity();
                modelMatrix.translate(x * gpucubeedge, y * gpucubeedge, z * gpucubeedge);
                if (xz) {
                    modelMatrix.rotate(90.0f, QVector3D(1.0f, 0.0f, 0.0f));
                } else if (zy) {
                    modelMatrix.rotate(90.0f, QVector3D(0.0f, -1.0f, 0.0f));
                }
                modelMatrix.scale(gpucubeedge);
                modelMatrix.scale(1, 1, 1);

                if (layer.isOverlayData) {
                    auto & punned = static_cast<gpu_lut_cube&>(ptr);
                    punned.cube.bind(0);
                    punned.lut.bind(1);
                    overlay_data_shader.setUniformValue("lutSize", static_cast<float>(punned.lut.width() * punned.lut.height() * punned.lut.depth()));
                    overlay_data_shader.setUniformValue("model_matrix", modelMatrix);
                } else {
                    ptr.cube.bind(0);
                    raw_data_shader.setUniformValue("model_matrix", modelMatrix);
                }
                glDrawArrays(GL_QUADS, 0, 4);
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
    if(options.selectionBuffer) {
        glDisable(GL_DEPTH_TEST);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glClear(GL_DEPTH_BUFFER_BIT); /* better place? TDitem */

    if(!state->viewerState->selectModeFlag) {
        if(state->viewerState->multisamplingOnOff) {
            glEnable(GL_MULTISAMPLE);
        }

        if(state->viewerState->lightOnOff) {
            /* Configure light. optimize that! TDitem */
            glEnable(GL_LIGHTING);
            GLfloat ambientLight[] = {0.5, 0.5, 0.5, 0.8};
            GLfloat diffuseLight[] = {1., 1., 1., 1.};
            GLfloat lightPos[] = {0., 0., 1., 1.};

            glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
            glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
            glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
            glEnable(GL_LIGHT0);

            GLfloat global_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

            /* Enable materials with automatic color assignment */
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
        }
    }

    /* Multiplying by state->magnification increases the area covered
     * by the textured OpenGL quad for downsampled datasets. */
    float dataPxX = texture.displayedEdgeLengthX / texture.texUnitsPerDataPx * 0.5;
    float dataPxY = texture.displayedEdgeLengthY / texture.texUnitsPerDataPx * 0.5;

    const bool xy = viewportType == VIEWPORT_XY;
    const bool xz = viewportType == VIEWPORT_XZ;
    const bool zy = viewportType == VIEWPORT_YZ;
    const bool arb = viewportType == VIEWPORT_ARBITRARY;
    if (!arb) {
        if (!state->viewerState->selectModeFlag) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
        }

        auto view = [&](){
            gluLookAt(state->viewerState->currentPosition.x, state->viewerState->currentPosition.y, state->viewerState->currentPosition.z
                    , state->viewerState->currentPosition.x - zy, state->viewerState->currentPosition.y - xz , state->viewerState->currentPosition.z + xy
                    , 0, -(xy || zy), -xz);
        };

        if (zy) {
            glOrtho(-dataPxY, +dataPxY, -dataPxX, +dataPxX, -state->viewerState->depthCutOff, +state->viewerState->depthCutOff);
        } else {
            glOrtho(-dataPxX, +dataPxX, -dataPxY, +dataPxY, -state->viewerState->depthCutOff, +state->viewerState->depthCutOff);
        }

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        view();

        updateFrustumClippingPlanes();

        glTranslatef(state->viewerState->currentPosition.x, state->viewerState->currentPosition.y, state->viewerState->currentPosition.z);
        glRotatef(180, 1, 0, 0);//OGL to K origin
        if (xz) {
            glRotatef(90, 1, 0, 0);
        } else if (zy) {
            glRotatef(90, 0, 1, 0);
        }

        auto swapYZ = [&]() {//TODO fix offsets everywhere
            if (zy) {
                std::swap(dataPxX, dataPxY);
            }
            std::swap(state->viewer->window->viewport(VIEWPORT_YZ)->texture.texRUx, state->viewer->window->viewport(VIEWPORT_YZ)->texture.texLLx);
            std::swap(state->viewer->window->viewport(VIEWPORT_YZ)->texture.texRUy, state->viewer->window->viewport(VIEWPORT_YZ)->texture.texLLy);
        };

        if(state->viewerState->selectModeFlag) {
            glLoadName(3);//tell the picking function that we render the slice here
        }

        glDisable(GL_DEPTH_TEST);//render first raw slice below everything

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1, 1, 1, 1);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture.texHandle);

        swapYZ();

        auto slice = [&](){
            glBegin(GL_QUADS);
                glNormal3i(0, 0, 1);
                glTexCoord2f(texture.texLUx, texture.texLUy);
                glVertex3f(-dataPxX, dataPxY, 0);
                glTexCoord2f(texture.texRUx, texture.texRUy);
                glVertex3f(dataPxX, dataPxY, 0);
                glTexCoord2f(texture.texRLx, texture.texRLy);
                glVertex3f(dataPxX, -dataPxY, 0);
                glTexCoord2f(texture.texLLx, texture.texLLy);
                glVertex3f(-dataPxX, -dataPxY, 0);
            glEnd();
        };

        slice();

        glBindTexture (GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        swapYZ();

        if (options.drawSkeleton && state->viewerState->skeletonDisplay.testFlag(SkeletonDisplay::ShowInOrthoVPs)) {
            glPushMatrix();
            glLoadIdentity();
            view();

            glTranslatef((xy || xz) * 0.5, (xy || zy) * 0.5, (xz || zy) * 0.5);//arrange to pixel center
            renderSkeleton(options);

            glPopMatrix();
        }

        swapYZ();

        if(state->viewerState->selectModeFlag) {
            glLoadName(3);//tell the picking function that we render slices again
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1, 1, 1, 0.6);//second raw slice is semi transparent, with one direction of the skeleton showing through and the other above rendered above

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, texture.texHandle);
        slice();

        /* Draw the overlay textures */
        if(options.drawOverlay) {
            glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);
            slice();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_DEPTH_TEST);
        if (options.drawCrosshairs) {
            glLineWidth(1);
            glBegin(GL_LINES);
                glColor4f(xz || zy, xy, 0, 0.3);
                glVertex3f(-dataPxX, -0.5, 0);//why negative 0.5 here?
                glVertex3f( dataPxX, -0.5, 0);

                glColor4f(0, zy, xy || xz , 0.3);
                glVertex3f(0.5, -dataPxY, 0);
                glVertex3f(0.5,  dataPxY, 0);
            glEnd();
        }

        swapYZ();

        if (Session::singleton().annotationMode.testFlag(AnnotationMode::Brush)) {
            glPushMatrix();
            glLoadIdentity();
            view();

            renderBrush(id, getMouseCoordinate());

            glPopMatrix();
        }

        glDepthFunc(GL_LESS);//reset depth func to default value
    } else {
        if(!state->viewerState->selectModeFlag) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
        }
        // left, right, bottom, top, near, far clipping planes
        glOrtho(-((float)(state->boundary.x)/ 2.) + (float)state->viewerState->currentPosition.x - dataPxX,
            -((float)(state->boundary.x) / 2.) + (float)state->viewerState->currentPosition.x + dataPxX,
            -((float)(state->boundary.y) / 2.) + (float)state->viewerState->currentPosition.y - dataPxY,
            -((float)(state->boundary.y) / 2.) + (float)state->viewerState->currentPosition.y + dataPxY,
            ((float)(state->boundary.z) / 2.) - state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z,
            ((float)(state->boundary.z) / 2.) + state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // optimize that! TDitem

        glTranslatef(-((float)state->boundary.x / 2.),
                    -((float)state->boundary.y / 2.),
                    -((float)state->boundary.z / 2.));

        glTranslatef((float)state->viewerState->currentPosition.x,
                    (float)state->viewerState->currentPosition.y,
                    (float)state->viewerState->currentPosition.z);

        floatCoordinate normal;
        floatCoordinate vec1;
        if (id == 0) {
            glRotatef(180., 1.,0.,0.);
            normal = {0, 0, 1};
            vec1 = {1, 0, 0};
        }

        else if (id == 1) {
            glRotatef(90., 1., 0., 0.);
            normal = {0, 1, 0};
            vec1 = {1, 0, 0};
        }

        else if (id == 2){
            glRotatef(90., 0., 1., 0.);
            glScalef(1., -1., 1.);
            normal = {1, 0, 0};
            vec1 = {0, 0, 1};
        }
        // first rotation makes the viewport face the camera
        float scalar = scalarProduct(normal, n);
        float angle = acosf(std::min(1.f, std::max(-1.f, scalar))); // deals with float imprecision at interval boundaries
        floatCoordinate axis = crossProduct(normal, n);
        if(normalizeVector(axis)) {
            glRotatef(-(angle*180/boost::math::constants::pi<float>()), axis.x, axis.y, axis.z);
        } // second rotation also aligns the plane vectors with the camera
        rotateAndNormalize(vec1, axis, angle);
        scalar = scalarProduct(vec1, v1);
        angle = acosf(std::min(1.f, std::max(-1.f, scalar)));
        axis = crossProduct(vec1, v1);
        if(normalizeVector(axis)) {
            glRotatef(-(angle*180/boost::math::constants::pi<float>()), axis.x, axis.y, axis.z);
        }

        glLoadName(3);

        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1., 1., 1., 1.);

        glBindTexture(GL_TEXTURE_2D, texture.texHandle);
        glBegin(GL_QUADS);
            glNormal3i(n.x, n.y, n.z);
            glTexCoord2f(texture.texLUx, texture.texLUy);
            glVertex3f(-dataPxX * v1.x - dataPxY * v2.x, -dataPxX * v1.y - dataPxY * v2.y, -dataPxX * v1.z - dataPxY * v2.z);
            glTexCoord2f(texture.texRUx, texture.texRUy);
            glVertex3f(dataPxX * v1.x - dataPxY * v2.x, dataPxX * v1.y - dataPxY * v2.y, dataPxX * v1.z - dataPxY * v2.z);
            glTexCoord2f(texture.texRLx, texture.texRLy);
            glVertex3f(dataPxX * v1.x + dataPxY * v2.x, dataPxX * v1.y + dataPxY * v2.y, dataPxX * v1.z + dataPxY * v2.z);
            glTexCoord2f(texture.texLLx, texture.texLLy);
            glVertex3f(-dataPxX * v1.x + dataPxY * v2.x, -dataPxX * v1.y + dataPxY * v2.y, -dataPxX * v1.z + dataPxY * v2.z);
        glEnd();
        glBindTexture (GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);

        if (options.drawSkeleton && state->viewerState->skeletonDisplay.testFlag(SkeletonDisplay::ShowInOrthoVPs)) {
            glPushMatrix();
            glTranslatef(-state->viewerState->currentPosition.x, -state->viewerState->currentPosition.y, -state->viewerState->currentPosition.z);
            glTranslatef(0.5, 0.5, 0.5);//arrange to pixel center, this is never correct, TODO angle adjustments
            renderSkeleton(options);
            glPopMatrix();
        }
        glLoadName(3);

        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1., 1., 1., 0.6);

        glBindTexture(GL_TEXTURE_2D, texture.texHandle);
        glBegin(GL_QUADS);
            glNormal3i(n.x, n.y, n.z);
            const auto offset = id == 1 ? n * -1 : n;
            glTexCoord2f(texture.texLUx, texture.texLUy);
            glVertex3f(-dataPxX * v1.x - dataPxY * v2.x + offset.x, -dataPxX * v1.y - dataPxY * v2.y + offset.y, -dataPxX * v1.z - dataPxY * v2.z + offset.z);
            glTexCoord2f(texture.texRUx, texture.texRUy);
            glVertex3f(dataPxX * v1.x - dataPxY * v2.x + offset.x, dataPxX * v1.y - dataPxY * v2.y + offset.y, dataPxX * v1.z - dataPxY * v2.z + offset.z);
            glTexCoord2f(texture.texRLx, texture.texRLy);

            glVertex3f(dataPxX * v1.x + dataPxY * v2.x + offset.x, dataPxX * v1.y + dataPxY * v2.y + offset.y, dataPxX * v1.z + dataPxY * v2.z + offset.z);
            glTexCoord2f(texture.texLLx, texture.texLLy);

            glVertex3f(-dataPxX * v1.x + dataPxY * v2.x + offset.x, -dataPxX * v1.y + dataPxY * v2.y + offset.y, -dataPxX * v1.z + dataPxY * v2.z + offset.z);
        glEnd();

        // Draw the overlay textures
        if(options.drawOverlay) {
            glBindTexture(GL_TEXTURE_2D, texture.overlayHandle);
            glBegin(GL_QUADS);
                glNormal3i(n.x, n.y, n.z);
                const auto offset = id == 1 ? n * 0.1 : n * -0.1;
                glTexCoord2f(texture.texLUx, texture.texLUy);
                glVertex3f(-dataPxX * v1.x - dataPxY * v2.x + offset.x,
                           -dataPxX * v1.y - dataPxY * v2.y + offset.y,
                           -dataPxX * v1.z - dataPxY * v2.z + offset.z);
                glTexCoord2f(texture.texRUx, texture.texRUy);
                glVertex3f(dataPxX * v1.x - dataPxY * v2.x + offset.x,
                           dataPxX * v1.y - dataPxY * v2.y + offset.y,
                           dataPxX * v1.z - dataPxY * v2.z + offset.z);
                glTexCoord2f(texture.texRLx, texture.texRLy);
                glVertex3f(dataPxX * v1.x + dataPxY * v2.x + offset.x,
                           dataPxX * v1.y + dataPxY * v2.y + offset.y,
                           dataPxX * v1.z + dataPxY * v2.z + offset.z);
                glTexCoord2f(texture.texLLx, texture.texLLy);
                glVertex3f(-dataPxX * v1.x + dataPxY * v2.x + offset.x,
                           -dataPxX * v1.y + dataPxY * v2.y + offset.y,
                           -dataPxX * v1.z + dataPxY * v2.z + offset.z);
            glEnd();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_DEPTH_TEST);
        if (options.drawCrosshairs) {
            glLineWidth(1.);
            glBegin(GL_LINES);
                glColor4f(v2.z, v2.y, v2.x, 0.3);
                glVertex3f(-dataPxX * v1.x + 0.5 * v2.x - 0.0001 * n.x,
                           -dataPxX * v1.y + 0.5 * v2.y - 0.0001 * n.y,
                           -dataPxX * v1.z + 0.5 * v2.z - 0.0001 * n.z);

                glVertex3f(dataPxX * v1.x + 0.5 * v2.x - 0.0001 * n.x,
                           dataPxX * v1.y + 0.5 * v2.y - 0.0001 * n.y,
                           dataPxX * v1.z + 0.5 * v2.z - 0.0001 * n.z);

                glColor4f(v1.z, v1.y, v1.x, 0.3);
                glVertex3f(0.5 * v1.x - dataPxY * v2.x - 0.0001 * n.x,
                           0.5 * v1.y - dataPxY * v2.y - 0.0001 * n.y,
                           0.5 * v1.z - dataPxY * v2.z - 0.0001 * n.z);

                glVertex3f(0.5 * v1.x + dataPxY * v2.x - 0.0001 * n.x,
                           0.5 * v1.y + dataPxY * v2.y - 0.0001 * n.y,
                           0.5 * v1.z + dataPxY * v2.z - 0.0001 * n.z);
            glEnd();
        }
    }

    glDisable(GL_BLEND);
}

bool Viewport3D::renderVolumeVP() {
    auto& seg = Segmentation::singleton();

    std::array<double, 3> background_color;
    seg.volume_background_color.getRgbF(&background_color[0], &background_color[1], &background_color[2]);
    glClearColor(background_color[0], background_color[1], background_color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(seg.volume_tex_id != 0) {
        static float volumeClippingAdjust = 1.73f;
        static float translationSpeedAdjust = 1.0 / 500.0f;
        auto cubeLen = state->cubeEdgeLength;
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
        auto datascale = state->scale;
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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        glDisable(GL_BLEND);
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

    return true;
}

void Viewport3D::renderViewport(const RenderOptions &options) {
    auto& seg = Segmentation::singleton();
    if (seg.volume_render_toggle) {
        if(seg.volume_update_required) {
            seg.volume_update_required = false;
            updateVolumeTexture();
        }
        renderVolumeVP();
    } else {
        renderSkeletonVP(options);
    }
}

bool Viewport3D::renderSkeletonVP(const RenderOptions &options) {
    if(!state->viewerState->selectModeFlag) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
    }
    // left, right, bottom, top, near, far clipping planes; substitute arbitrary vals to something more sensible. TDitem
    glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
        state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
        state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
        state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY, -1000, 10 *state->skeletonState->volBoundary);

    screenPxXPerDataPx = (float)edgeLength / (state->skeletonState->volBoundary - 2.f * state->skeletonState->volBoundary * state->skeletonState->zoomLevel);
    displayedlengthInNmX = edgeLength / screenPxXPerDataPx * state->scale.x;

    if(state->viewerState->lightOnOff) {
        // Configure light
        glEnable(GL_LIGHTING);
        GLfloat ambientLight[] = {0.5, 0.5, 0.5, 0.8};
        GLfloat diffuseLight[] = {1., 1., 1., 1.};
        GLfloat lightPos[] = {0., 0., 1., 1.};

        glLightfv(GL_LIGHT0,GL_AMBIENT,ambientLight);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuseLight);
        glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
        glEnable(GL_LIGHT0);

        GLfloat global_ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

        // Enable materials with automatic color tracking
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    }

    if(state->viewerState->multisamplingOnOff) {
        glEnable(GL_MULTISAMPLE);
    }
     // Now we set up the view on the skeleton and draw some very basic VP stuff like the gray background
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

     // Now we draw the  background of our skeleton VP

    glPushMatrix();
    glTranslatef(0., 0., -10. * ((float)state->skeletonState->volBoundary - 2.));

    glShadeModel(GL_SMOOTH);
    glDisable(GL_TEXTURE_2D);

    glLoadName(1);

    glColor4f(1., 1., 1., 1.); // HERE
    // The * 10 should prevent, that the user translates into space with gray background - dirty solution. TDitem
    glBegin(GL_QUADS);
        glVertex3i(-state->skeletonState->volBoundary * 10, -state->skeletonState->volBoundary * 10, 0);
        glVertex3i(state->skeletonState->volBoundary  * 10, -state->skeletonState->volBoundary * 10, 0);
        glVertex3i(state->skeletonState->volBoundary  * 10, state->skeletonState->volBoundary  * 10, 0);
        glVertex3i(-state->skeletonState->volBoundary * 10, state->skeletonState->volBoundary  * 10, 0);
    glEnd();

    glPopMatrix();

    // load model view matrix that stores rotation state!
    glLoadMatrixf(state->skeletonState->skeletonVpModelView);

    // perform user defined coordinate system rotations. use single matrix multiplication as opt.! TDitem
    if(state->skeletonState->rotdx || state->skeletonState->rotdy) {
        if((state->viewerState->rotateAroundActiveNode) && (state->skeletonState->activeNode)) {
            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->skeletonState->activeNode->position.x,
                         (float)state->skeletonState->activeNode->position.y,
                         (float)state->skeletonState->activeNode->position.z);
            glScalef(1., 1., state->viewerState->voxelXYtoZRatio);
            rotateViewport();
            glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
            glTranslatef(-(float)state->skeletonState->activeNode->position.x,
                         -(float)state->skeletonState->activeNode->position.y,
                         -(float)state->skeletonState->activeNode->position.z);
            glTranslatef(((float)state->boundary.x / 2.),((float)state->boundary.y / 2.),((float)state->boundary.z / 2.));
        }
        // rotate around dataset center if no active node is selected
        else {
            glScalef(1., 1., state->viewerState->voxelXYtoZRatio);
            rotateViewport();
            glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
        }
        // save the modified basic model view matrix
        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);
    }

    switch(state->skeletonState->definedSkeletonVpView) {
    case SKELVP_XY_VIEW:
        state->skeletonState->definedSkeletonVpView = -1;

        glLoadIdentity();
        glTranslatef((float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / -2.);

        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        state->skeletonState->translateX = ((float)state->boundary.x / -2.) + (float)state->viewerState->currentPosition.x;
        state->skeletonState->translateY = ((float)state->boundary.y / -2.) + (float)state->viewerState->currentPosition.y;

        glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
                state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY,
                -500,
                10 * state->skeletonState->volBoundary);
        setRotationState(ROTATIONSTATEXY);
        break;
    case SKELVP_XZ_VIEW:
        state->skeletonState->definedSkeletonVpView = -1;
        glLoadIdentity();
        glTranslatef((float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / -2.);
        glRotatef(-90, 1., 0., 0.);
        glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);

        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        state->skeletonState->translateX = ((float)state->boundary.x / -2.) + (float)state->viewerState->currentPosition.x;
        state->skeletonState->translateY = ((float)state->boundary.z / -2.) + (float)state->viewerState->currentPosition.z;

        glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
                state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY,
                -500,
                10 * state->skeletonState->volBoundary);
        setRotationState(ROTATIONSTATEXZ);
        break;
    case SKELVP_YZ_VIEW:
        state->skeletonState->definedSkeletonVpView = -1;
        glLoadIdentity();
        glTranslatef((float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / -2.);
        glRotatef(90, 0., 1., 0.);
        glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        state->skeletonState->translateX = ((float)state->boundary.z / -2.) + (float)state->viewerState->currentPosition.z;
        state->skeletonState->translateY = ((float)state->boundary.y / -2.) + (float)state->viewerState->currentPosition.y;

        glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
                state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
                state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY,
                -500,
                10 * state->skeletonState->volBoundary);
        setRotationState(ROTATIONSTATEYZ);
        break;
    case SKELVP_R90:
        state->skeletonState->rotdx = 10;
        state->skeletonState->rotationcounter++;
        if (state->skeletonState->rotationcounter > 15) {
            state->skeletonState->rotdx = 7.6;
            state->skeletonState->definedSkeletonVpView = -1;
            state->skeletonState->rotationcounter = 0;
        }
        break;
    case SKELVP_R180:
        state->skeletonState->rotdx = 10;
        state->skeletonState->rotationcounter++;
        if (state->skeletonState->rotationcounter > 31) {
            state->skeletonState->rotdx = 5.2;
            state->skeletonState->definedSkeletonVpView = -1;
            state->skeletonState->rotationcounter = 0;
        }
        break;
    case SKELVP_RESET:
        state->skeletonState->definedSkeletonVpView = -1;
        state->skeletonState->translateX = 0;
        state->skeletonState->translateY = 0;
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef((float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / 2.,
                     (float)state->skeletonState->volBoundary / -2.);
        glScalef(-1., 1., 1.);
        glRotatef(235., 1., 0., 0.);
        glRotatef(210., 0., 0., 1.);
        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);
        state->skeletonState->zoomLevel = SKELZOOMMIN;
        setRotationState(ROTATIONSTATERESET);
        break;
    default:
        break;
    }

    if(options.drawViewportPlanes) { // Draw the slice planes for orientation inside the data stack
        glPushMatrix();

        // single operation! TDitem
        glTranslatef(-((float)state->boundary.x / 2.), -((float)state->boundary.y / 2.), -((float)state->boundary.z / 2.));
        glTranslatef(0.5, 0.5, 0.5);

        updateFrustumClippingPlanes();
        glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);

        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1., 1., 1., 1.);
        state->viewer->window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
            // Used for calculation of slice pane length inside the 3d view
            float dataPxX = orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx * 0.5;
            float dataPxY = orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx * 0.5;

            switch(orthoVP.viewportType) {
            case VIEWPORT_XY:
                if(!state->viewerState->showXYplane) break;
                glBindTexture(GL_TEXTURE_2D, orthoVP.texture.texHandle);
                glLoadName(VIEWPORT_XY);
                glBegin(GL_QUADS);
                    glNormal3i(0,0,1);
                    glTexCoord2f(orthoVP.texture.texLUx, orthoVP.texture.texLUy);
                    glVertex3f(-dataPxX, -dataPxY, 0.);
                    glTexCoord2f(orthoVP.texture.texRUx, orthoVP.texture.texRUy);
                    glVertex3f(dataPxX, -dataPxY, 0.);
                    glTexCoord2f(orthoVP.texture.texRLx, orthoVP.texture.texRLy);
                    glVertex3f(dataPxX, dataPxY, 0.);
                    glTexCoord2f(orthoVP.texture.texLLx, orthoVP.texture.texLLy);
                    glVertex3f(-dataPxX, dataPxY, 0.);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                break;
            case VIEWPORT_XZ:
                if(!state->viewerState->showXZplane) break;
                glBindTexture(GL_TEXTURE_2D, orthoVP.texture.texHandle);
                glLoadName(VIEWPORT_XZ);
                glBegin(GL_QUADS);
                    glNormal3i(0,1,0);
                    glTexCoord2f(orthoVP.texture.texLUx, orthoVP.texture.texLUy);
                    glVertex3f(-dataPxX, 0., -dataPxY);
                    glTexCoord2f(orthoVP.texture.texRUx, orthoVP.texture.texRUy);
                    glVertex3f(dataPxX, 0., -dataPxY);
                    glTexCoord2f(orthoVP.texture.texRLx, orthoVP.texture.texRLy);
                    glVertex3f(dataPxX, 0., dataPxY);
                    glTexCoord2f(orthoVP.texture.texLLx, orthoVP.texture.texLLy);
                    glVertex3f(-dataPxX, 0., dataPxY);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                break;
            case VIEWPORT_YZ:
                if(!state->viewerState->showYZplane) break;
                glBindTexture(GL_TEXTURE_2D, orthoVP.texture.texHandle);
                glLoadName(VIEWPORT_YZ);
                glBegin(GL_QUADS);
                    glNormal3i(1,0,0);
                    glTexCoord2f(orthoVP.texture.texLUx, orthoVP.texture.texLUy);
                    glVertex3f(0., -dataPxX, -dataPxY);
                    glTexCoord2f(orthoVP.texture.texRUx, orthoVP.texture.texRUy);
                    glVertex3f(0., dataPxX, -dataPxY);
                    glTexCoord2f(orthoVP.texture.texRLx, orthoVP.texture.texRLy);
                    glVertex3f(0., dataPxX, dataPxY);
                    glTexCoord2f(orthoVP.texture.texLLx, orthoVP.texture.texLLy);
                    glVertex3f(0., -dataPxX, dataPxY);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                break;
            }
        });
        state->viewer->window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
            if (viewportType == VIEWPORT_ARBITRARY) {
                if ( (orthoVP.id == VP_UPPERLEFT && state->viewerState->showXYplane)
                    || (orthoVP.id == VP_LOWERLEFT && state->viewerState->showXZplane)
                    || (orthoVP.id == VP_UPPERRIGHT && state->viewerState->showYZplane) )
                {
                    renderArbitrarySlicePane(orthoVP);
                }
            }
        });

        glDisable(GL_TEXTURE_2D);

        state->viewer->window->forEachOrthoVPDo([this](ViewportOrtho & orthoVP) {
            GLUquadricObj * gluCylObj;
            float dataPxX = orthoVP.texture.displayedEdgeLengthX / orthoVP.texture.texUnitsPerDataPx * 0.5;
            float dataPxY = orthoVP.texture.displayedEdgeLengthY / orthoVP.texture.texUnitsPerDataPx * 0.5;
            switch(orthoVP.viewportType) {
            case VIEWPORT_XY:
                glColor4f(0.7, 0., 0., 1.);
                glBegin(GL_LINE_LOOP);
                    glVertex3f(-dataPxX, -dataPxY, 0.);
                    glVertex3f(dataPxX, -dataPxY, 0.);
                    glVertex3f(dataPxX, dataPxY, 0.);
                    glVertex3f(-dataPxX, dataPxY, 0.);
                glEnd();

                glColor4f(0., 0., 0., 1.);
                glPushMatrix();
                glTranslatef(-dataPxX, 0., 0.);
                glRotatef(90., 0., 1., 0.);
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                gluCylinder(gluCylObj, 0.4, 0.4, dataPxX * 2, 5, 5);
                gluDeleteQuadric(gluCylObj);
                glPopMatrix();

                glPushMatrix();
                glTranslatef(0., dataPxY, 0.);
                glRotatef(90., 1., 0., 0.);
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                gluCylinder(gluCylObj, 0.4, 0.4, dataPxY * 2, 5, 5);
                gluDeleteQuadric(gluCylObj);
                glPopMatrix();

                break;
            case VIEWPORT_XZ:
                glColor4f(0., 0.7, 0., 1.);
                glBegin(GL_LINE_LOOP);
                    glVertex3f(-dataPxX, 0., -dataPxY);
                    glVertex3f(dataPxX, 0., -dataPxY);
                    glVertex3f(dataPxX, 0., dataPxY);
                    glVertex3f(-dataPxX, 0., dataPxY);
                glEnd();

                glColor4f(0., 0., 0., 1.);
                glPushMatrix();
                glTranslatef(0., 0., -dataPxY);
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                gluCylinder(gluCylObj, 0.4, 0.4, dataPxY * 2, 5, 5);
                gluDeleteQuadric(gluCylObj);
                glPopMatrix();

                break;
            case VIEWPORT_YZ:
                glColor4f(0., 0., 0.7, 1.);
                glBegin(GL_LINE_LOOP);
                    glVertex3f(0., -dataPxX, -dataPxY);
                    glVertex3f(0., dataPxX, -dataPxY);
                    glVertex3f(0., dataPxX, dataPxY);
                    glVertex3f(0., -dataPxX, dataPxY);
                glEnd();
                break;
            case VIEWPORT_ARBITRARY:
                glColor4f(orthoVP.n.z, orthoVP.n.y, orthoVP.n.x, 1.);

                glBegin(GL_LINE_LOOP);
                    glVertex3f(-dataPxX * orthoVP.v1.x - dataPxY * orthoVP.v2.x,
                               -dataPxX * orthoVP.v1.y - dataPxY * orthoVP.v2.y,
                               -dataPxX * orthoVP.v1.z - dataPxY * orthoVP.v2.z);
                    glVertex3f(dataPxX * orthoVP.v1.x - dataPxY * orthoVP.v2.x,
                               dataPxX * orthoVP.v1.y - dataPxY * orthoVP.v2.y,
                               dataPxX * orthoVP.v1.z - dataPxY * orthoVP.v2.z);
                    glVertex3f(dataPxX * orthoVP.v1.x + dataPxY * orthoVP.v2.x,
                               dataPxX * orthoVP.v1.y + dataPxY * orthoVP.v2.y,
                               dataPxX * orthoVP.v1.z + dataPxY * orthoVP.v2.z);
                    glVertex3f(-dataPxX * orthoVP.v1.x + dataPxY * orthoVP.v2.x,
                               -dataPxX * orthoVP.v1.y + dataPxY * orthoVP.v2.y,
                               -dataPxX * orthoVP.v1.z + dataPxY * orthoVP.v2.z);
                glEnd();
                break;
            }
        });

        glPopMatrix();
        glEnable(GL_TEXTURE_2D);
    }

    if(options.drawBoundaryBox || options.drawBoundaryAxes) {
        // Now we draw the dataset corresponding stuff (volume box of right size, axis descriptions...)
        glEnable(GL_BLEND);

        if(options.drawBoundaryBox) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLoadName(3);
            glColor4f(0.8, 0.8, 0.8, 1.0);
            glBegin(GL_QUADS);
                glNormal3i(0,0,1);
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), -(state->boundary.z / 2));

                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), -(state->boundary.z / 2));

                glNormal3i(0,0,1);
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), (state->boundary.z / 2));

                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), (state->boundary.z / 2));

                glNormal3i(0,1,0);
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), (state->boundary.z / 2));

                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), -(state->boundary.z / 2));

                glNormal3i(0,1,0);
                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), (state->boundary.z / 2));

                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), -(state->boundary.z / 2));

                glNormal3i(1,0,0);
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), (state->boundary.z / 2));

                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(-(state->boundary.x / 2), (state->boundary.y / 2), -(state->boundary.z / 2));

                glNormal3i(1,0,0);
                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), -(state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, -(state->boundary.y / 2), (state->boundary.z / 2));

                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), (state->boundary.z / 2));
                glVertex3i(state->boundary.x / 2, (state->boundary.y / 2), -(state->boundary.z / 2));
            glEnd();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // draw axes
        GLdouble origin[3], conePos_x[3], conePos_y[3], conePos_z[3], model[16], projection[16];
        GLint gl_viewport[4];
        glGetDoublev(GL_MODELVIEW_MATRIX, &model[0]);
        glGetDoublev(GL_PROJECTION_MATRIX, &projection[0]);
        glGetIntegerv(GL_VIEWPORT, gl_viewport);
        gluProject(-state->boundary.x/2, -state->boundary.y/2, -state->boundary.z/2, &model[0], &projection[0], &gl_viewport[0], &origin[0], &origin[1], &origin[2]);
        gluProject(state->boundary.x/2, -state->boundary.y/2, -state->boundary.z/2, &model[0], &projection[0], &gl_viewport[0], &conePos_x[0], &conePos_x[1], &conePos_x[2]);
        gluProject(-state->boundary.x/2, state->boundary.y/2, -state->boundary.z/2, &model[0], &projection[0], &gl_viewport[0], &conePos_y[0], &conePos_y[1], &conePos_y[2]);
        gluProject(-state->boundary.x/2, -state->boundary.y/2, state->boundary.z/2, &model[0], &projection[0], &gl_viewport[0], &conePos_z[0], &conePos_z[1], &conePos_z[2]);
        floatCoordinate axis_x{static_cast<float>(conePos_x[0] - origin[0]), static_cast<float>(origin[1] - conePos_x[1]), static_cast<float>(conePos_x[2] - origin[2])};
        floatCoordinate axis_y{static_cast<float>(conePos_y[0] - origin[0]), static_cast<float>(origin[1] - conePos_y[1]), static_cast<float>(conePos_y[2] - origin[2])};
        floatCoordinate axis_z{static_cast<float>(conePos_z[0] - origin[0]), static_cast<float>(origin[1] - conePos_z[1]), static_cast<float>(conePos_z[2] - origin[2])};
        glDisable(GL_DEPTH_TEST);
        auto renderAxis = [this, gl_viewport](floatCoordinate & targetView, const QString label) {
            glPushMatrix();
            floatCoordinate currentView = {0, 0, -1};
            const auto angle = acosf(scalarProduct(currentView, targetView));
            auto axis = crossProduct(currentView, targetView);
            if (normalizeVector(axis)) {
                glRotatef(-(angle*180/boost::math::constants::pi<float>()), axis.x, axis.y, axis.z);
            }
            // axis
            const auto diameter = std::ceil(0.005*gl_viewport[2]);
            GLUquadricObj * gluCylObj = gluNewQuadric();
            gluQuadricNormals(gluCylObj, GLU_SMOOTH);
            gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
            gluCylinder(gluCylObj, diameter, diameter , euclidicNorm(targetView), 10, 1);
            gluDeleteQuadric(gluCylObj);
            // cone z
            glTranslatef(0, 0, euclidicNorm(targetView));
            gluCylObj = gluNewQuadric();
            gluQuadricNormals(gluCylObj, GLU_SMOOTH);
            gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
            gluCylinder(gluCylObj, std::ceil(0.014*gl_viewport[2]), 0., std::ceil(0.028*gl_viewport[2]), 10, 5);
            gluDeleteQuadric(gluCylObj);
            const int offset = std::ceil(0.043*gl_viewport[2]);
            renderText({offset, offset, offset}, label, std::ceil(0.02*gl_viewport[2]));
            glPopMatrix();
        };
        // remember world coordinate system
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        // switch to viewport coordinate system
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const auto lenLongestAxis = std::max(euclidicNorm(axis_x), std::max(euclidicNorm(axis_y), euclidicNorm(axis_z)));
        glOrtho(0, gl_viewport[2], gl_viewport[3], 0, lenLongestAxis, -lenLongestAxis);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(origin[0], gl_viewport[3] - origin[1], origin[2]);
        glColor4f(0, 0, 0, 1);
        if(Viewport3D::showBoundariesInUm) {
            renderAxis(axis_x, QString("x: %1 µm").arg(state->boundary.x * state->scale.x * 0.001));
            renderAxis(axis_y, QString("y: %1 µm").arg(state->boundary.y * state->scale.y * 0.001));
            renderAxis(axis_z, QString("z: %1 µm").arg(state->boundary.z * state->scale.z * 0.001));
        }
        else {
            renderAxis(axis_x, QString("x: %1 px").arg(state->boundary.x + 1));
            renderAxis(axis_y, QString("y: %1 px").arg(state->boundary.y + 1));
            renderAxis(axis_z, QString("z: %1 px").arg(state->boundary.z + 1));
        }
        // restore world coordinate system
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    if(options.drawSkeleton && state->viewerState->skeletonDisplay.testFlag(SkeletonDisplay::ShowIn3DVP)) {
        glPushMatrix();
        glTranslatef(-state->boundary.x / 2., -state->boundary.y / 2., -state->boundary.z / 2.);//reset to origin of projection
        glTranslatef(0.5, 0.5, 0.5);//arrange to pixel center
        renderSkeleton(options);
        glPopMatrix();
    }

     // Reset previously changed OGL parameters
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glLoadIdentity();
    return true;
}

void ViewportOrtho::renderBrush(uint viewportType, Coordinate coord) {
    glLineWidth(2.0f);

    auto & seg = Segmentation::singleton();
    auto drawCursor = [this, &seg, viewportType, coord]() {
        const auto bradius = seg.brush.getRadius();
        const auto bview = seg.brush.getView();
        const auto xsize = bradius / state->scale.x;
        const auto ysize = bradius / state->scale.y;
        const auto zsize = bradius / state->scale.z;

        //move from center to cursor
        glTranslatef(coord.x, coord.y, coord.z);
        if (viewportType == VIEWPORT_XZ && bview == brush_t::view_t::xz) {
            glTranslatef(0, 0, 1);//move origin to other corner of voxel, idrk why that’s necessary
            glRotatef(-90, 1, 0, 0);
        } else if(viewportType == VIEWPORT_YZ && bview == brush_t::view_t::yz) {
            glTranslatef(0, 0, 1);//move origin to other corner of voxel, idrk why that’s necessary
            glRotatef(90, 0, 1, 0);
        } else if (viewportType != VIEWPORT_XY || bview != brush_t::view_t::xy) {
            return;
        }

        const bool xy = viewportType == VIEWPORT_XY;
        const bool xz = viewportType == VIEWPORT_XZ;
        const int z = 0;

        if(seg.brush.getShape() == brush_t::shape_t::angular) {
            glBegin(GL_LINE_LOOP);
            const auto x = xy || xz ? xsize : zsize;
            const auto y = xz ? zsize : ysize;
            //integer coordinates to round to voxel boundaries
            glVertex3i(-x    , -y    , z);
            glVertex3i( x + 1, -y    , z);
            glVertex3i( x + 1,  y + 1, z);
            glVertex3i(-x    ,  y + 1, z);
            glEnd();
        } else if(seg.brush.getShape() == brush_t::shape_t::round) {
            const int xmax = xy ? xsize : xz ? xsize : zsize;
            const int ymax = xy ? ysize : xz ? zsize : ysize;
            int y = 0;
            int x = xmax;
            auto verticalPixelBorder = [this](float x, float y, float z){
                glVertex3f(x, y    , z);
                glVertex3f(x, y + 1, z);
            };
            auto horizontalPixelBorder = [this](float x, float y, float z){
                glVertex3f(x    , y, z);
                glVertex3f(x + 1, y, z);
            };

            glBegin(GL_LINES);
            while (x >= y) {//first part of the ellipse (circle with anisotropic pixels), y dominant movement
                auto val = isInsideSphere(xy ? x : xz ? x : z, xy ? y : xz ? z : y, xy ? z : xz ? y : x, bradius);
                if (val) {
                    verticalPixelBorder( x + 1,  y, z);
                    verticalPixelBorder(-x    ,  y, z);
                    verticalPixelBorder(-x    , -y, z);
                    verticalPixelBorder( x + 1, -y, z);
                } else if (x != xmax || y != 0) {
                    horizontalPixelBorder( x,  y    , z);
                    horizontalPixelBorder(-x,  y    , z);
                    horizontalPixelBorder(-x, -y + 1, z);
                    horizontalPixelBorder( x, -y + 1, z);
                }
                if (val) {
                    ++y;
                } else {
                    --x;
                }
            }

            x = 0;
            y = ymax;
            while (y >= x) {//second part of the ellipse, x dominant movement
                auto val = isInsideSphere(xy ? x : xz ? x : z, xy ? y : xz ? z : y, xy ? z : xz ? y : x, bradius);
                if (val) {
                    horizontalPixelBorder( x,  y + 1, z);
                    horizontalPixelBorder(-x,  y + 1, z);
                    horizontalPixelBorder(-x, -y    , z);
                    horizontalPixelBorder( x, -y    , z);
                } else if (y != ymax || x != 0) {
                    verticalPixelBorder( x    ,  y, z);
                    verticalPixelBorder(-x + 1,  y, z);
                    verticalPixelBorder(-x + 1, -y, z);
                    verticalPixelBorder( x    , -y, z);
                }
                if (val) {
                    ++x;
                } else {
                    --y;
                }
            }
            glEnd();
        }
    };

    if (seg.brush.isInverse()) {
        glColor3f(1.0f, 0.1f, 0.1f);
    } else {
        glColor3f(0.2f, 0.2f, 0.2f);
    }
    drawCursor();
}

void Viewport3D::renderArbitrarySlicePane(const ViewportOrtho & vp) {
    // Used for calculation of slice pane length inside the 3d view
    const auto dataPxX = vp.texture.displayedEdgeLengthX / vp.texture.texUnitsPerDataPx * 0.5;
    const auto dataPxY = vp.texture.displayedEdgeLengthY / vp.texture.texUnitsPerDataPx * 0.5;

    glLoadName(vp.id);//for select mode

    glBindTexture(GL_TEXTURE_2D, vp.texture.texHandle);

    glBegin(GL_QUADS);
        glNormal3i(vp.n.x, vp.n.y, vp.n.z);
        glTexCoord2f(vp.texture.texLUx, vp.texture.texLUy);
        glVertex3f(-dataPxX * vp.v1.x - dataPxY * vp.v2.x, -dataPxX * vp.v1.y - dataPxY * vp.v2.y, -dataPxX * vp.v1.z - dataPxY * vp.v2.z);
        glTexCoord2f(vp.texture.texRUx, vp.texture.texRUy);
        glVertex3f(dataPxX * vp.v1.x - dataPxY * vp.v2.x, dataPxX * vp.v1.y - dataPxY * vp.v2.y, dataPxX * vp.v1.z - dataPxY * vp.v2.z);
        glTexCoord2f(vp.texture.texRLx, vp.texture.texRLy);
        glVertex3f(dataPxX * vp.v1.x + dataPxY * vp.v2.x, dataPxX * vp.v1.y + dataPxY * vp.v2.y, dataPxX * vp.v1.z + dataPxY * vp.v2.z);
        glTexCoord2f(vp.texture.texLLx, vp.texture.texLLy);
        glVertex3f(-dataPxX * vp.v1.x + dataPxY * vp.v2.x, -dataPxX * vp.v1.y + dataPxY * vp.v2.y, -dataPxX * vp.v1.z + dataPxY * vp.v2.z);
    glEnd();
    glBindTexture (GL_TEXTURE_2D, 0);
}

boost::optional<nodeListElement &> ViewportBase::retrieveVisibleObjectBeneathSquare(uint x, uint y, uint width) {
    const auto & nodes = retrieveAllObjectsBeneathSquare(x, y, width, width);
    if (nodes.size() != 0) {
        return *(*std::begin(nodes));
    }
    return boost::none;
}

QSet<nodeListElement *> ViewportBase::retrieveAllObjectsBeneathSquare(uint centerX, uint centerY, uint selectionWidth, uint selectionHeight) {
    makeCurrent();

    //4 elems per node: hit_count(always 1), min, max and 1 name
    //generous amount of addional space for non-node-glloadname-calls
    std::vector<GLuint> selectionBuffer(state->skeletonState->totalNodeElements * 4 + 2048);
    glSelectBuffer(selectionBuffer.size(), selectionBuffer.data());

    state->viewerState->selectModeFlag = true;
    glRenderMode(GL_SELECT);

    glInitNames();
    glPushName(0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLdouble vp_height = height();

    if(id != VIEWPORT_SKELETON) {
        vp_height = height() * devicePixelRatio();
    }
    centerX *= devicePixelRatio();
    centerY *= devicePixelRatio();
    selectionWidth *= devicePixelRatio();
    selectionHeight *= devicePixelRatio();

    GLint openGLviewport[4];
    glGetIntegerv(GL_VIEWPORT, openGLviewport);

    gluPickMatrix(centerX, vp_height - centerY, selectionWidth, selectionHeight, openGLviewport);

    RenderOptions options;
    options.selectionBuffer = true;
    renderViewport(options);

    GLint hits = glRenderMode(GL_RENDER);
    glLoadIdentity();

    qDebug() << "selection hits: " << hits;
    QSet<nodeListElement *> foundNodes;
    for (std::size_t i = 0; i < selectionBuffer.size();) {
        if (hits == 0) {//if hits was positive and reaches 0
            //if overflow bit was set hits is negative and we only use the buffer-end-condition
            break;
        }
        --hits;

        const GLuint hit_count = selectionBuffer[i];
        if (hit_count > 0) {
            const GLuint name = selectionBuffer[i+3];//the first name on the stack is the 4th element of the hit record
            if (name >= GLNAME_NODEID_OFFSET) {
                nodeListElement * const foundNode = Skeletonizer::findNodeByNodeID(name - GLNAME_NODEID_OFFSET);
                if (foundNode != nullptr) {
                    foundNodes.insert(foundNode);
                }
            }
        }
        i = i + 3 + hit_count;
    }
    state->viewerState->selectModeFlag = false;

    return foundNodes;
}

bool Viewport3D::updateRotationStateMatrix(float M1[16], float M2[16]){
    //multiply matrix m2 to matrix m1 and save result in rotationState matrix
    int i;
    float M3[16];

    M3[0] = M1[0] * M2[0] + M1[4] * M2[1] + M1[8] * M2[2] + M1[12] * M2[3];
    M3[1] = M1[1] * M2[0] + M1[5] * M2[1] + M1[9] * M2[2] + M1[13] * M2[3];
    M3[2] = M1[2] * M2[0] + M1[6] * M2[1] + M1[10] * M2[2] + M1[14] * M2[3];
    M3[3] = M1[3] * M2[0] + M1[7] * M2[1] + M1[11] * M2[2] + M1[15] * M2[3];
    M3[4] = M1[0] * M2[4] + M1[4] * M2[5] + M1[8] * M2[6] + M1[12] * M2[7];
    M3[5] = M1[1] * M2[4] + M1[5] * M2[5] + M1[9] * M2[6] + M1[13] * M2[7];
    M3[6] = M1[2] * M2[4] + M1[6] * M2[5] + M1[10] * M2[6] + M1[14] * M2[7];
    M3[7] = M1[3] * M2[4] + M1[7] * M2[5] + M1[11] * M2[6] + M1[15] * M2[7];
    M3[8] = M1[0] * M2[8] + M1[4] * M2[9] + M1[8] * M2[10] + M1[12] * M2[11];
    M3[9] = M1[1] * M2[8] + M1[5] * M2[9] + M1[9] * M2[10] + M1[13] * M2[11];
    M3[10] = M1[2] * M2[8] + M1[6] * M2[9] + M1[10] * M2[10] + M1[14] * M2[11];
    M3[11] = M1[3] * M2[8] + M1[7] * M2[9] + M1[11] * M2[10] + M1[15] * M2[11];
    M3[12] = M1[0] * M2[12] + M1[4] * M2[13] + M1[8] * M2[14] + M1[12] * M2[15];
    M3[13] = M1[1] * M2[12] + M1[5] * M2[13] + M1[9] * M2[14] + M1[13] * M2[15];
    M3[14] = M1[2] * M2[12] + M1[6] * M2[13] + M1[10] * M2[14] + M1[14] * M2[15];
    M3[15] = M1[3] * M2[12] + M1[7] * M2[13] + M1[11] * M2[14] + M1[15] * M2[15];

    for (i = 0; i < 16; i++){
        state->skeletonState->rotationState[i] = M3[i];
    }
    return true;
}

bool Viewport3D::rotateViewport() {
    // for general information look at http://de.wikipedia.org/wiki/Rolling-Ball-Rotation

    // rotdx and rotdy save the small rotations the user creates with one single mouse action
    // singleRotM[16] is the rotation matrix for this single mouse action (see )
    // state->skeletonstate->rotationState is the product of all rotations during KNOSSOS session
    // inverseRotationState is the inverse (here transposed) matrix of state->skeletonstate->rotationState

    float singleRotM[16];
    float inverseRotationState[16];
    float rotR = 100.;
    float rotdx = (float)state->skeletonState->rotdx;
    float rotdy = (float)state->skeletonState->rotdy;
    state->skeletonState->rotdx = 0;
    state->skeletonState->rotdy = 0;
    float rotdr = pow(rotdx * rotdx + rotdy * rotdy, 0.5);
    float rotCosT = rotR / (pow(rotR * rotR + rotdr * rotdr, 0.5));
    float rotSinT = rotdr / (pow(rotR * rotR + rotdr * rotdr, 0.5));

    //calc inverse matrix of actual rotation state
    inverseRotationState[0] = state->skeletonState->rotationState[0];
    inverseRotationState[1] = state->skeletonState->rotationState[4];
    inverseRotationState[2] = state->skeletonState->rotationState[8];
    inverseRotationState[3] = state->skeletonState->rotationState[12];
    inverseRotationState[4] = state->skeletonState->rotationState[1];
    inverseRotationState[5] = state->skeletonState->rotationState[5];
    inverseRotationState[6] = state->skeletonState->rotationState[9];
    inverseRotationState[7] = state->skeletonState->rotationState[13];
    inverseRotationState[8] = state->skeletonState->rotationState[2];
    inverseRotationState[9] = state->skeletonState->rotationState[6];
    inverseRotationState[10] = state->skeletonState->rotationState[10];
    inverseRotationState[11] = state->skeletonState->rotationState[14];
    inverseRotationState[12] = state->skeletonState->rotationState[3];
    inverseRotationState[13] = state->skeletonState->rotationState[7];
    inverseRotationState[14] = state->skeletonState->rotationState[11];
    inverseRotationState[15] = state->skeletonState->rotationState[15];

    // calc matrix of one single rotation
    singleRotM[0] = rotCosT + pow(rotdy / rotdr, 2.) * (1. - rotCosT);
    singleRotM[1] = rotdx * rotdy / rotdr / rotdr * (rotCosT - 1.);
    singleRotM[2] = - rotdx / rotdr * rotSinT;
    singleRotM[3] = 0.;
    singleRotM[4] = singleRotM[1];
    singleRotM[5] = rotCosT + pow(rotdx / rotdr, 2.) * (1. - rotCosT);
    singleRotM[6] = - rotdy / rotdr * rotSinT;
    singleRotM[7] = 0.;
    singleRotM[8] = - singleRotM[2];
    singleRotM[9] = - singleRotM[6];
    singleRotM[10] = rotCosT;
    singleRotM[11] = 0.;
    singleRotM[12] = 0.;
    singleRotM[13] = 0.;
    singleRotM[14] = 0.;
    singleRotM[15] = 1.;

    // undo all previous rotations
    glMultMatrixf(inverseRotationState);

    // multiply all previous rotations to current rotation and overwrite state->skeletonState->rotationsState
    updateRotationStateMatrix(singleRotM,state->skeletonState->rotationState);

    //rotate to the new rotation state
    glMultMatrixf(state->skeletonState->rotationState);

    return true;
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
    treeListElement *currentTree;
    nodeListElement *currentNode, *lastNode = NULL, *lastRenderedNode = NULL;
    segmentListElement *currentSegment;
    float cumDistToLastRenderedNode;
    floatCoordinate currNodePos;
    uint virtualSegRendered, allowHeuristic;

    state->skeletonState->lineVertBuffer.vertsIndex = 0;
    state->skeletonState->lineVertBuffer.normsIndex = 0;
    state->skeletonState->lineVertBuffer.colsIndex = 0;

    state->skeletonState->pointVertBuffer.vertsIndex = 0;
    state->skeletonState->pointVertBuffer.normsIndex = 0;
    state->skeletonState->pointVertBuffer.colsIndex = 0;
    color4F currentColor = {1.f, 0.f, 0.f, 1.f};

    //tdItem: test culling under different conditions!
    //if(viewportType == VIEWPORT_SKELETON) glEnable(GL_CULL_FACE);

    /* Enable blending just once, since we never disable it? */
    glEnable(GL_BLEND);

    glPushMatrix();

    /* We iterate over the whole tree structure. */
    currentTree = state->skeletonState->firstTree.get();

    while(currentTree) {

        /* Render only trees we want to be rendered*/
        if(!currentTree->render) {
            currentTree = currentTree->next.get();
            continue;
        }

        lastNode = NULL;
        lastRenderedNode = NULL;
        cumDistToLastRenderedNode = 0.f;

        if(state->viewerState->skeletonDisplay.testFlag(SkeletonDisplay::OnlySelected) && !currentTree->selected) {
            currentTree = currentTree->next.get();
            continue;
        }

        currentNode = currentTree->firstNode.get();
        while(currentNode) {

            /* We start with frustum culling:
             * all nodes that are not in the current viewing frustum for the
             * currently rendered viewports are discarded. This is very fast. */

            /* For frustum culling. These values should be stored, mem <-> cpu tradeoff  */
            currNodePos.x = (float)currentNode->position.x;
            currNodePos.y = (float)currentNode->position.y;
            currNodePos.z = (float)currentNode->position.z;

            /* Every node is tested based on a precomputed circumsphere
            that includes its segments. */

            if(!sphereInFrustum(currNodePos, currentNode->circRadius)) {
                currentNode = currentNode->next.get();
                lastNode = lastRenderedNode = NULL;
                continue;
            }

            virtualSegRendered = false;
            bool nodeVisible = true;

            /* First test whether this node is actually connected to the next,
            i.e. whether the implicit sorting is not broken here. */
            allowHeuristic = false;
            if(currentNode->next && (!(currentNode->numSegs > 2))) {
                currentSegment = currentNode->next->firstSegment;
                while(currentSegment) {

                    if((currentSegment->target == currentNode) ||
                       (currentSegment->source == currentNode)) {
                        /* Connected, heuristic is allowed */
                        allowHeuristic = true;
                        break;
                    }
                    currentSegment = currentSegment->next;
                }
            }


            currentSegment = currentNode->firstSegment;
            while(currentSegment && allowHeuristic && !state->viewerState->selectModeFlag) {
                /* isBranchNode tells you only whether the node is on the branch point stack,
                 * not whether it is actually a node connected to more than two other nodes! */
                if((currentSegment->target == lastNode)
                    || ((currentSegment->source == lastNode)
                    &&
                    (!(
                       (currentNode->comment)
                       || (currentNode->isBranchNode)
                       || (currentNode->numSegs > 2)
                       || (currentNode->radius * screenPxXPerDataPx > 5.f))))) {

                    /* Node is a candidate for LOD culling */

                    /* Do we really skip this node? Test cum dist. to last rendered node! */
                    cumDistToLastRenderedNode += currentSegment->length * screenPxXPerDataPx;

                    if(cumDistToLastRenderedNode > state->viewerState->cumDistRenderThres) {
                        break;
                    }
                    else {
                        nodeVisible = false;
                        break;
                    }
                }
                currentSegment = currentSegment->next;
            }

            if(nodeVisible) {
                /* This sets the current color for the segment rendering */
                if((currentTree->treeID == state->skeletonState->activeTree->treeID)
                    && (state->viewerState->highlightActiveTree)) {
                        currentColor = {1.f, 0.f, 0.f, 1.f};
                }
                else {
                    currentColor = currentTree->color;
                }

                cumDistToLastRenderedNode = 0.f;

                if(lastNode != lastRenderedNode) {
                    virtualSegRendered = true;
                    // We need a "virtual" segment now
                    if(state->viewerState->selectModeFlag) {
                        glLoadName(3);
                    }
                    segmentListElement virtualSegment;
                    virtualSegment.source = lastRenderedNode;
                    virtualSegment.target = currentNode;
                    renderSegment(virtualSegment, currentColor);
                }

                /* Second pass over segments needed... But only if node is actually rendered! */
                currentSegment = currentNode->firstSegment;
                while(currentSegment) {
                    if(currentSegment->flag == SEGMENT_BACKWARD){
                        currentSegment = currentSegment->next;
                        continue;
                    }

                    if(virtualSegRendered && (currentSegment->source == lastNode || currentSegment->target == lastNode)) {
                        currentSegment = currentSegment->next;
                        continue;
                    }

                    if(state->viewerState->selectModeFlag) {
                        glLoadName(3);
                    }
                    renderSegment(*currentSegment, currentColor);
                    currentSegment = currentSegment->next;
                }

                if(state->viewerState->selectModeFlag) {
                    glLoadName(GLNAME_NODEID_OFFSET + currentNode->nodeID);
                }
                renderNode(*currentNode, options);

                lastRenderedNode = currentNode;
            }

            lastNode = currentNode;

            currentNode = currentNode->next.get();
        }

        currentTree = currentTree->next.get();
    }

    if(state->viewerState->selectModeFlag)
        glLoadName(3);

    /* Render line geometry batch if it contains data */
    if(state->skeletonState->lineVertBuffer.vertsIndex > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        /* draw all segments */
        glVertexPointer(3, GL_FLOAT, 0, state->skeletonState->lineVertBuffer.vertices);
        glColorPointer(4, GL_FLOAT, 0, state->skeletonState->lineVertBuffer.colors);

        glDrawArrays(GL_LINES, 0, state->skeletonState->lineVertBuffer.vertsIndex);

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    if(state->viewerState->overrideNodeRadiusBool)
        glPointSize(state->viewerState->overrideNodeRadiusVal);
    else
        glPointSize(3.f);

    /* Render point geometry batch if it contains data */
    if(state->skeletonState->pointVertBuffer.vertsIndex > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        /* draw all segments */
        glVertexPointer(3, GL_FLOAT, 0, state->skeletonState->pointVertBuffer.vertices);
        glColorPointer(4, GL_FLOAT, 0, state->skeletonState->pointVertBuffer.colors);

        glDrawArrays(GL_POINTS, 0, state->skeletonState->pointVertBuffer.vertsIndex);

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

    }
    glPointSize(1.f);

    glPopMatrix(); // Restore modelview matrix
    glEnable(GL_BLEND);
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

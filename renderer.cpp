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

#include "renderer.h"
#include "functions.h"
#include "widgets/viewport.h"
#include <math.h>

#include <qgl.h>
#ifdef Q_OS_MACX
    #include <glu.h>
#endif
#ifdef Q_OS_LINUX
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif
#ifdef Q_OS_WIN
    #include <GL/glu.h>
#endif
#include "skeletonizer.h"
#include "viewer.h"

extern stateInfo *state;

Renderer::Renderer(QObject *parent) :
    QObject(parent)
{
    font = QFont("Helvetica", 10, QFont::Normal);

    uint i;
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        /* Initialize the basic model view matrix for the skeleton VP
        Perform basic coordinate system rotations */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glTranslatef((float)state->skeletonState->volBoundary / 2.,
            (float)state->skeletonState->volBoundary / 2.,
            -((float)state->skeletonState->volBoundary / 2.));

        glScalef(-1., 1., 1.);
        //);
        //LOG("state->viewerState->voxelXYtoZRatio = %f", state->viewerState->voxelXYtoZRatio)
        glRotatef(235., 1., 0., 0.);
        glRotatef(210., 0., 0., 1.);
        setRotationState(ROTATIONSTATERESET);
        //glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
        /* save the matrix for further use... */
        glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

        glLoadIdentity();

        // get a unique display list identifier for each viewport
        for(i = 0; i < state->viewerState->numberViewports; i++) {
            state->viewerState->vpConfigs[i].displayList = glGenLists(1);
        }

        initMesh(&(state->skeletonState->lineVertBuffer), 1024);
        initMesh(&(state->skeletonState->pointVertBuffer), 1024);

}

uint Renderer::renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius, color4F color, uint viewportType) {
    float currentAngle = 0.;
        floatCoordinate segDirection, tempVec, *tempVec2;
        GLUquadricObj *gluCylObj = NULL;


        if(((state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx
            * baseRadius < 1.f)
           && (state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx
            * topRadius < 1.f)) || (state->viewerState->cumDistRenderThres > 19.f)) {

            if(state->skeletonState->lineVertBuffer.vertsBuffSize < state->skeletonState->lineVertBuffer.vertsIndex + 2)
                doubleMeshCapacity(&(state->skeletonState->lineVertBuffer));

            SET_COORDINATE(state->skeletonState->lineVertBuffer.vertices[state->skeletonState->lineVertBuffer.vertsIndex], (float)base->x, (float)base->y, (float)base->z);
            SET_COORDINATE(state->skeletonState->lineVertBuffer.vertices[state->skeletonState->lineVertBuffer.vertsIndex + 1], (float)top->x, (float)top->y, (float)top->z);

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
            tempVec.x = 0.;
            tempVec.y = 0.;
            tempVec.z = 1.;
            segDirection.x = (float)(top->x - base->x);
            segDirection.y = (float)(top->y - base->y);
            segDirection.z = (float)(top->z - base->z);

            //temVec2 defines the rotation axis
            tempVec2 = crossProduct(&tempVec, &segDirection);
            currentAngle = radToDeg(vectorAngle(&tempVec, &segDirection));

            //some gl implementations have problems with the params occuring for
            //segs in straight directions. we need a fix here.
            glRotatef(currentAngle, tempVec2->x, tempVec2->y, tempVec2->z);

            free(tempVec2);

            //tdItem use state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx for proper LOD
            //the same values have to be used in rendersegplaneintersections to avoid ugly graphics
            if((baseRadius > 100.f) || topRadius > 100.f) {
                gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(&segDirection), 10, 1);
            }
            else if((baseRadius > 15.f) || topRadius > 15.f) {
                gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(&segDirection), 6, 1);
            }
            else {
                gluCylinder(gluCylObj, baseRadius, topRadius, euclidicNorm(&segDirection), 3, 1);
            }

            gluDeleteQuadric(gluCylObj);
            glPopMatrix();
        }

        return true;
}

uint Renderer::renderSphere(Coordinate *pos, float radius, color4F color, uint viewportType) {
    GLUquadricObj *gluSphereObj = NULL;

        /* Render only a point if the sphere wouldn't be visible anyway */

        if((state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx
           * radius > 0.0f) && (state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx
           * radius < 2.0f) || (state->viewerState->cumDistRenderThres > 19.f)) {

            /* This is cumbersome, but SELECT mode cannot be used with glDrawArray.
            Color buffer picking brings its own issues on the other hand, so we
            stick with SELECT mode for the time being. */
            if(state->viewerState->selectModeFlag) {
                glPointSize(radius * 2.);
                glBegin(GL_POINTS);
                    glVertex3f((float)pos->x, (float)pos->y, (float)pos->z);
                glEnd();
                glPointSize(1.);
            }
            else {
                if(state->skeletonState->pointVertBuffer.vertsBuffSize < state->skeletonState->pointVertBuffer.vertsIndex + 2)
                    doubleMeshCapacity(&(state->skeletonState->pointVertBuffer));

                SET_COORDINATE(state->skeletonState->pointVertBuffer.vertices[state->skeletonState->pointVertBuffer.vertsIndex], (float)pos->x, (float)pos->y, (float)pos->z);
                state->skeletonState->pointVertBuffer.colors[state->skeletonState->pointVertBuffer.vertsIndex] = color;
                state->skeletonState->pointVertBuffer.vertsIndex++;
            }
        }
        else {
            GLfloat tmp[] = {color.r, color.g, color.b, color.a};
            glColor4fv(tmp);
            glPushMatrix();
            glTranslatef((float)pos->x, (float)pos->y, (float)pos->z);
            glScalef(1.f, 1.f, state->viewerState->voxelXYtoZRatio);
            gluSphereObj = gluNewQuadric();
            gluQuadricNormals(gluSphereObj, GLU_SMOOTH);
            gluQuadricOrientation(gluSphereObj, GLU_OUTSIDE);

            if(radius * state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx  > 20.) {
                gluSphere(gluSphereObj, radius, 14, 14);
            }
            else if(radius * state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx  > 5.) {
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


uint Renderer::renderText(Coordinate *pos, const char *string, uint viewportType) {

    glDisable(GL_DEPTH_TEST);
    glRasterPos3d(pos->x, pos->y, pos->z);

    if(viewportType == VIEWPORT_XY)
        refVPXY->renderText(pos->x, pos->y, pos->z, QString(string), font);
    else if(viewportType == VIEWPORT_XZ)
        refVPXZ->renderText(pos->x, pos->y, pos->z, QString(string), font);
    else if(viewportType == VIEWPORT_YZ)
         refVPYZ->renderText(pos->x, pos->y, pos->z, QString(string), font);
    else if(viewportType == VIEWPORT_SKELETON)
        refVPSkel->renderText(pos->x, pos->y, pos->z, QString(string), font);

    glEnable(GL_DEPTH_TEST);


    return true;
}

uint Renderer::renderSegPlaneIntersection(struct segmentListElement *segment) {
    if(!state->skeletonState->showIntersections) return true;

        float p[2][3], a, currentAngle, length, radius, distSourceInter, sSr_local, sTr_local;
        int i, distToCurrPos;
        floatCoordinate *tempVec2, tempVec, tempVec3, segDir, intPoint, sTp_local, sSp_local;
        GLUquadricObj *gluCylObj = NULL;

        sSp_local.x = (float)segment->source->position.x;
        sSp_local.y = (float)segment->source->position.y;
        sSp_local.z = (float)segment->source->position.z;

        sTp_local.x = (float)segment->target->position.x;
        sTp_local.y = (float)segment->target->position.y;
        sTp_local.z = (float)segment->target->position.z;

        sSr_local = (float)segment->source->radius;
        sTr_local = (float)segment->target->radius;

        //n contains the normal vectors of the 3 orthogonal planes
        float n[3][3] = {{1.,0.,0.},
                        {0.,1.,0.},
                        {0.,0.,1.}};

        distToCurrPos = ((state->M/2)+1)
            + 1 * state->cubeEdgeLength;

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

                    length = euclidicNorm(&segDir);
                    distSourceInter = euclidicNorm(&tempVec3);

                    if(sSr_local < sTr_local)
                        radius = sTr_local + sSr_local * (1. - distSourceInter / length);
                    else if(sSr_local == sTr_local)
                        radius = sSr_local;
                    else
                        radius = sSr_local - (sSr_local - sTr_local) * distSourceInter / length;

                    segDir.x /= length;
                    segDir.y /= length;
                    segDir.z /= length;

                    glTranslatef(intPoint.x - 0.75 * segDir.x, intPoint.y - 0.75 * segDir.y, intPoint.z - 0.75 * segDir.z);
                    //glTranslatef(intPoint.x, intPoint.y, intPoint.z);

                    //Some calculations for the correct direction of the cylinder.
                    tempVec.x = 0.;
                    tempVec.y = 0.;
                    tempVec.z = 1.;

                    //temVec2 defines the rotation axis
                    tempVec2 = crossProduct(&tempVec, &segDir);
                    currentAngle = radToDeg(vectorAngle(&tempVec, &segDir));
                    glRotatef(currentAngle, tempVec2->x, tempVec2->y, tempVec2->z);
                    free(tempVec2);

                    glColor4f(0.,0.,0.,1.);

                    if(state->skeletonState->overrideNodeRadiusBool)
                        gluCylinder(gluCylObj,
                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius*1.2,
                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius*1.2,
                            1.5, 3, 1);

                    else gluCylinder(gluCylObj,
                            radius * state->skeletonState->segRadiusToNodeRadius*1.2,
                            radius * state->skeletonState->segRadiusToNodeRadius*1.2,
                            1.5, 3, 1);

                    gluDeleteQuadric(gluCylObj);
                    glPopMatrix();
                }

            }
        }

    return true;

}

uint Renderer::renderViewportBorders(uint currentVP) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* define coordinate system for our viewport: left right bottom top near far */
    glOrtho(0, state->viewerState->vpConfigs[currentVP].edgeLength,
            state->viewerState->vpConfigs[currentVP].edgeLength, 0, 25, -25);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    switch(state->viewerState->vpConfigs[currentVP].type) {
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
            /* arb */
            glColor4f(state->viewerState->vpConfigs[currentVP].n.z, state->viewerState->vpConfigs[currentVP].n.y, state->viewerState->vpConfigs[currentVP].n.x, 1.);

        break;
    }
    glLineWidth(3.);
    glBegin(GL_LINES);
        glVertex3d(2, 1, -1);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, 1, -1);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, 1, -1);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, state->viewerState->vpConfigs[currentVP].edgeLength - 1, -1);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, state->viewerState->vpConfigs[currentVP].edgeLength - 1, -1);
        glVertex3d(2, state->viewerState->vpConfigs[currentVP].edgeLength - 2, -1);
        glVertex3d(2, state->viewerState->vpConfigs[currentVP].edgeLength - 2, -1);
        glVertex3d(2, 1, -1);
    glEnd();

    if(state->viewerState->vpConfigs[currentVP].type == state->viewerState->highlightVp) {
        // Draw an orange border to highlight the viewport.

        glColor4f(1., 0.3, 0., 1.);
        glBegin(GL_LINES);
            glVertex3d(5, 4, -1);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, 4, -1);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, 4, -1);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, state->viewerState->vpConfigs[currentVP].edgeLength - 4, -1);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, state->viewerState->vpConfigs[currentVP].edgeLength - 4, -1);
            glVertex3d(5, state->viewerState->vpConfigs[currentVP].edgeLength - 5, -1);
            glVertex3d(5, state->viewerState->vpConfigs[currentVP].edgeLength - 5, -1);
            glVertex3d(5, 4, -1);
        glEnd();
    }

    glLineWidth(1.);

    return true;
}


// Currently not used
/* @todo update from trunk */
static uint overlayOrthogonalVpPixel(uint currentVP, Coordinate position, color4F color)  {

}


bool Renderer::renderOrthogonalVP(uint currentVP) {
    float dataPxX, dataPxY;
    //for displaying data size
    float width, height;
    char label[1024];
    Coordinate pos;

    floatCoordinate *n, *v1, *v2;

    n = &(state->viewerState->vpConfigs[currentVP].n);

    v1 = &(state->viewerState->vpConfigs[currentVP].v1);

    v2 = &(state->viewerState->vpConfigs[currentVP].v2);

    if(!((state->viewerState->vpConfigs[currentVP].type == VIEWPORT_XY)
            || (state->viewerState->vpConfigs[currentVP].type == VIEWPORT_XZ)
            || (state->viewerState->vpConfigs[currentVP].type == VIEWPORT_YZ)
         || (state->viewerState->vpConfigs[currentVP].type == VIEWPORT_ARBITRARY))) {
       LOG("Wrong VP type given for renderOrthogonalVP() call.")
        return false;
    }



    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glClear(GL_DEPTH_BUFFER_BIT); /* better place? TDitem */

    if(!state->viewerState->selectModeFlag) {
        if(state->viewerState->multisamplingOnOff) glEnable(GL_MULTISAMPLE);

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

    dataPxX = state->viewerState->vpConfigs[currentVP].texture.displayedEdgeLengthX
            / state->viewerState->vpConfigs[currentVP].texture.texUnitsPerDataPx
            * 0.5;
//            * (float)state->magnification;
    dataPxY = state->viewerState->vpConfigs[currentVP].texture.displayedEdgeLengthY
            / state->viewerState->vpConfigs[currentVP].texture.texUnitsPerDataPx
            * 0.5;
//            * (float)state->magnification;

    switch(state->viewerState->vpConfigs[currentVP].type) {
        case VIEWPORT_XY:
            if(!state->viewerState->selectModeFlag) {
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
            }
            /* left, right, bottom, top, near, far clipping planes */
            glOrtho(-((float)(state->boundary.x)/ 2.) + (float)state->viewerState->currentPosition.x - dataPxX,
                -((float)(state->boundary.x) / 2.) + (float)state->viewerState->currentPosition.x + dataPxX,
                -((float)(state->boundary.y) / 2.) + (float)state->viewerState->currentPosition.y - dataPxY,
                -((float)(state->boundary.y) / 2.) + (float)state->viewerState->currentPosition.y + dataPxY,
                ((float)(state->boundary.z) / 2.) - state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z,
                ((float)(state->boundary.z) / 2.) + state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            /*optimize that! TDitem */

            glTranslatef(-((float)state->boundary.x / 2.),
                        -((float)state->boundary.y / 2.),
                        -((float)state->boundary.z / 2.));

            updateFrustumClippingPlanes(VIEWPORT_XY);

            glTranslatef((float)state->viewerState->currentPosition.x,
                        (float)state->viewerState->currentPosition.y,
                        (float)state->viewerState->currentPosition.z);

            glRotatef(180., 1.,0.,0.);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 1.);
               // LOG("ortho VP tex XY id: %d", state->viewerState->vpConfigs[currentVP].texture.texHandle)
            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(0,0,1);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(-dataPxX, -dataPxY, 0.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(dataPxX, -dataPxY, 0.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(dataPxX, dataPxY, 0.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(-dataPxX, dataPxY, 0.);
            glEnd();
            glBindTexture (GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_DEPTH_TEST);


            glTranslatef(-(float)state->viewerState->currentPosition.x, -(float)state->viewerState->currentPosition.y, -(float)state->viewerState->currentPosition.z);
            glTranslatef(((float)state->boundary.x / 2.),((float)state->boundary.y / 2.),((float)state->boundary.z / 2.));

            renderSkeleton(VIEWPORT_XY);

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 0.6);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(0,0,1);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(-dataPxX, -dataPxY, 1.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(dataPxX, -dataPxY, 1.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(dataPxX, dataPxY, 1.);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(-dataPxX, dataPxY, 1.);
            glEnd();

            /* Draw the overlay textures */
            if(state->overlay) {
                if(state->viewerState->overlayVisible) {
                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.overlayHandle);
                    glBegin(GL_QUADS);
                        glNormal3i(0, 0, 1);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texLUy);
                        glVertex3f(-dataPxX, -dataPxY, -0.1);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texRUy);
                        glVertex3f(dataPxX, -dataPxY, -0.1);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx,
                                     state->viewerState->vpConfigs[currentVP].texture.texRLy);
                        glVertex3f(dataPxX, dataPxY, -0.1);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx,
                                     state->viewerState->vpConfigs[currentVP].texture.texLLy);
                        glVertex3f(-dataPxX, dataPxY, -0.1);
                    glEnd();
                }
            }

            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_DEPTH_TEST);

            if(state->viewerState->drawVPCrosshairs) {
                glLineWidth(1.);
                glBegin(GL_LINES);
                    glColor4f(0., 1., 0., 0.3);
                    glVertex3f(-dataPxX, 0.5, -0.0001);
                    glVertex3f(dataPxX, 0.5, -0.0001);

                    glColor4f(0., 0., 1., 0.3);
                    glVertex3f(0.5, -dataPxY, -0.0001);
                    glVertex3f(0.5, dataPxY, -0.0001);
                glEnd();
            }
            if(state->viewerState->showVPLabels) {
                glColor4f(0, 0, 0, 1);
                width = state->viewerState->vpConfigs[VIEWPORT_XY].displayedlengthInNmX*0.001;
                height = state->viewerState->vpConfigs[VIEWPORT_XY].displayedlengthInNmY*0.001;
                SET_COORDINATE(pos, -dataPxX + 30, -dataPxY + 30, -0.0001);
                memset(label, '\0', 1024);
                sprintf(label, "Height %.2f \u00B5m, Width %.2f \u00B5m", height, width);
                renderText(&pos, label, VIEWPORT_XY);
            }
            break;

        case VIEWPORT_XZ:
            if(!state->viewerState->selectModeFlag) {
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
            }
            /* left, right, bottom, top, near, far clipping planes */
            glOrtho(-((float)state->boundary.x / 2.) + (float)state->viewerState->currentPosition.x - dataPxX,
                -((float)state->boundary.x / 2.) + (float)state->viewerState->currentPosition.x + dataPxX,
                -((float)state->boundary.y / 2.) + (float)state->viewerState->currentPosition.y - dataPxY,
                -((float)state->boundary.y / 2.) + (float)state->viewerState->currentPosition.y + dataPxY,
                ((float)state->boundary.z / 2.) - state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z,
                ((float)state->boundary.z / 2.) + state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            /*optimize that! TDitem */

            glTranslatef(-((float)state->boundary.x / 2.),
                        -((float)state->boundary.y / 2.),
                        -((float)state->boundary.z / 2.));

            glTranslatef((float)state->viewerState->currentPosition.x,
                        (float)state->viewerState->currentPosition.y,
                        (float)state->viewerState->currentPosition.z);

            glRotatef(90., 1., 0., 0.);


            glTranslatef(-(float)state->viewerState->currentPosition.x,
                            -(float)state->viewerState->currentPosition.y,
                            -(float)state->viewerState->currentPosition.z);

            updateFrustumClippingPlanes(VIEWPORT_XZ);

            glTranslatef((float)state->viewerState->currentPosition.x,
                        (float)state->viewerState->currentPosition.y,
                        (float)state->viewerState->currentPosition.z);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);


            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 1.);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(0,1,0);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(-dataPxX, 0., -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(dataPxX, 0., -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(dataPxX, 0., dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(-dataPxX, 0., dataPxY);
            glEnd();
            glBindTexture (GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_DEPTH_TEST);

            glTranslatef(-(float)state->viewerState->currentPosition.x, -(float)state->viewerState->currentPosition.y, -(float)state->viewerState->currentPosition.z);
            glTranslatef(((float)state->boundary.x / 2.),((float)state->boundary.y / 2.),((float)state->boundary.z / 2.));


            renderSkeleton(VIEWPORT_XZ);



            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 0.6);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(0,1,0);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(-dataPxX, 0., -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(dataPxX, -0., -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(dataPxX, -0., dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(-dataPxX, -0., dataPxY);
            glEnd();

            /* Draw overlay */
            if(state->overlay) {
                if(state->viewerState->overlayVisible) {
                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.overlayHandle);
                    glBegin(GL_QUADS);
                        glNormal3i(0,1,0);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texLUy);
                        glVertex3f(-dataPxX, 0.1, -dataPxY);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texRUy);
                        glVertex3f(dataPxX, 0.1, -dataPxY);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx,
                                     state->viewerState->vpConfigs[currentVP].texture.texRLy);
                        glVertex3f(dataPxX, 0.1, dataPxY);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx,
                                     state->viewerState->vpConfigs[currentVP].texture.texLLy);
                        glVertex3f(-dataPxX, 0.1, dataPxY);
                    glEnd();
                }
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_DEPTH_TEST);

            if(state->viewerState->drawVPCrosshairs) {
                glLineWidth(1.);
                glBegin(GL_LINES);
                    glColor4f(1., 0., 0., 0.3);
                    glVertex3f(-dataPxX, 0.0001, 0.5);
                    glVertex3f(dataPxX, 0.0001, 0.5);

                    glColor4f(0., 0., 1., 0.3);
                    glVertex3f(0.5, 0.0001, -dataPxY);
                    glVertex3f(0.5, 0.0001, dataPxY);
                glEnd();
            }
            if(state->viewerState->showVPLabels) {
                glColor4f(0, 0, 0, 1);
                width = state->viewerState->vpConfigs[VIEWPORT_XZ].displayedlengthInNmX*0.001;
                height = state->viewerState->vpConfigs[VIEWPORT_XZ].displayedlengthInNmY*0.001;
                SET_COORDINATE(pos, -dataPxX + 30, 0.0001, -dataPxY + 30);
                memset(label, '\0', 1024);
                sprintf(label, "Height %.2f \u00B5m, Width %.2f \u00B5m", height, width);
                renderText(&pos, label, VIEWPORT_XZ);
            }
            break;
        case VIEWPORT_YZ:
            if(!state->viewerState->selectModeFlag) {
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
            }

            glOrtho(-((float)state->boundary.x / 2.) + (float)state->viewerState->currentPosition.x - dataPxY,
                -((float)state->boundary.x / 2.) + (float)state->viewerState->currentPosition.x + dataPxY,
                -((float)state->boundary.y / 2.) + (float)state->viewerState->currentPosition.y - dataPxX,
                -((float)state->boundary.y / 2.) + (float)state->viewerState->currentPosition.y + dataPxX,
                ((float)state->boundary.z / 2.) - state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z,
                ((float)state->boundary.z / 2.) + state->viewerState->depthCutOff - (float)state->viewerState->currentPosition.z);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);
            glRotatef(90., 0., 1., 0.);
            glScalef(1., -1., 1.);

            glTranslatef(-(float)state->viewerState->currentPosition.x,
                        -(float)state->viewerState->currentPosition.y,
                        -(float)state->viewerState->currentPosition.z);

            updateFrustumClippingPlanes(VIEWPORT_YZ);

            glTranslatef((float)state->viewerState->currentPosition.x,
                        (float)state->viewerState->currentPosition.y,
                        (float)state->viewerState->currentPosition.z);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 1.);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(1,0,0);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(0., -dataPxX, -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(0., dataPxX, -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(0., dataPxX, dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(0., -dataPxX, dataPxY);
            glEnd();
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_DEPTH_TEST);

            glTranslatef(-(float)state->viewerState->currentPosition.x, -(float)state->viewerState->currentPosition.y, -(float)state->viewerState->currentPosition.z);
            glTranslatef((float)state->boundary.x / 2.,(float)state->boundary.y / 2.,(float)state->boundary.z / 2.);

            renderSkeleton(VIEWPORT_YZ);

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);

            if(state->viewerState->selectModeFlag)
                glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 0.6);

            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
            glBegin(GL_QUADS);
                glNormal3i(1,0,0);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                glVertex3f(1., -dataPxX, -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                glVertex3f(1., dataPxX, -dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);
                glVertex3f(1., dataPxX, dataPxY);
                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);
                glVertex3f(1., -dataPxX, dataPxY);
            glEnd();

            /* Draw overlay */
            if(state->overlay) {
                if(state->viewerState->overlayVisible) {
                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.overlayHandle);
                    glBegin(GL_QUADS);
                        glNormal3i(1,0,0);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texLUy);
                        glVertex3f(-0.1, -dataPxX, -dataPxY);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx,
                                     state->viewerState->vpConfigs[currentVP].texture.texRUy);
                    glVertex3f(-0.1, dataPxX, -dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx,
                                 state->viewerState->vpConfigs[currentVP].texture.texRLy);
                    glVertex3f(-0.1, dataPxX, dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx,
                                 state->viewerState->vpConfigs[currentVP].texture.texLLy);
                    glVertex3f(-0.1, -dataPxX, dataPxY);
                    glEnd();
                }

            }

            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_DEPTH_TEST);

            if(state->viewerState->drawVPCrosshairs) {
                glLineWidth(1.);
                glBegin(GL_LINES);
                    glColor4f(1., 0., 0., 0.3);
                    glVertex3f(-0.0001, -dataPxX, 0.5);
                    glVertex3f(-0.0001, dataPxX, 0.5);

                    glColor4f(0., 1., 0., 0.3);
                    glVertex3f(-0.0001, 0.5, -dataPxX);
                    glVertex3f(-0.0001, 0.5, dataPxX);
                glEnd();
            }
            if(state->viewerState->showVPLabels) {
                glColor4f(0, 0, 0, 1);
                width = state->viewerState->vpConfigs[VIEWPORT_YZ].displayedlengthInNmX*0.001;
                height = state->viewerState->vpConfigs[VIEWPORT_YZ].displayedlengthInNmY*0.001;
                SET_COORDINATE(pos, -0.0001, -dataPxX + 30, -dataPxX + 30);
                memset(label, '\0', 1024);
                sprintf(label, "Height %.2f \u00B5m, Width %.2f \u00B5m", height, width);
                renderText(&pos, label, VIEWPORT_YZ);
            }
            break;
        case VIEWPORT_ARBITRARY:
        /* @arb */
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


                    //glRotatef(state->alpha, 0., 0., 1.); ?
                    //glRotatef(state->beta, 0., 1., 0.); ?

                    glMatrixMode(GL_MODELVIEW);
                    glLoadIdentity();

                    // optimize that! TDitem

                    glTranslatef(-((float)state->boundary.x / 2.),
                                -((float)state->boundary.y / 2.),
                                -((float)state->boundary.z / 2.));

                    glTranslatef((float)state->viewerState->currentPosition.x,
                                (float)state->viewerState->currentPosition.y,
                                (float)state->viewerState->currentPosition.z);

                    if (currentVP == 0) {
                        glRotatef(180., 1.,0.,0.);
                    }

                    else if (currentVP == 1) {
                        glRotatef(90., 1., 0., 0.);
                    }

                    else if (currentVP == 2){
                        glRotatef(90., 0., 1., 0.);
                        glScalef(1., -1., 1.);
                    }

                    glRotatef(-state->beta, 0., 1., 0.);
                    glRotatef(-state->alpha, 0., 0., 1.);
                    glLoadName(3);

                    glEnable(GL_TEXTURE_2D);
                    glDisable(GL_DEPTH_TEST);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glColor4f(1., 1., 1., 1.);

                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
                    glBegin(GL_QUADS);
                        glNormal3i(n->x, n->y, n->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                        glVertex3f(-dataPxX * v1->x - dataPxY * v2->x, -dataPxX * v1->y - dataPxY * v2->y, -dataPxX * v1->z - dataPxY * v2->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                        glVertex3f(dataPxX * v1->x - dataPxY * v2->x, dataPxX * v1->y - dataPxY * v2->y, dataPxX * v1->z - dataPxY * v2->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);

                        glVertex3f(dataPxX * v1->x + dataPxY * v2->x, dataPxX * v1->y + dataPxY * v2->y, dataPxX * v1->z + dataPxY * v2->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);

                        glVertex3f(-dataPxX * v1->x + dataPxY * v2->x, -dataPxX * v1->y + dataPxY * v2->y, -dataPxX * v1->z + dataPxY * v2->z);
                    glEnd();
                    glBindTexture (GL_TEXTURE_2D, 0);
                    glDisable(GL_TEXTURE_2D);
                    glEnable(GL_DEPTH_TEST);

                    glTranslatef(-(float)state->viewerState->currentPosition.x, -(float)state->viewerState->currentPosition.y, -(float)state->viewerState->currentPosition.z);
                    glTranslatef(((float)state->boundary.x / 2.),((float)state->boundary.y / 2.),((float)state->boundary.z / 2.));

                    if(state->skeletonState->displayListSkeletonSlicePlaneVP) glCallList(state->skeletonState->displayListSkeletonSlicePlaneVP);

                    glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
                    glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);
                    glLoadName(3);

                    glEnable(GL_TEXTURE_2D);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    glColor4f(1., 1., 1., 0.6);

                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.texHandle);
                    glBegin(GL_QUADS);
                        glNormal3i(n->x, n->y, n->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx, state->viewerState->vpConfigs[currentVP].texture.texLUy);
                        glVertex3f(-dataPxX * v1->x - dataPxY * v2->x + n->x, -dataPxX * v1->y - dataPxY * v2->y + n->y, -dataPxX * v1->z - dataPxY * v2->z + n->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx, state->viewerState->vpConfigs[currentVP].texture.texRUy);
                        glVertex3f(dataPxX * v1->x - dataPxY * v2->x + n->x, dataPxX * v1->y - dataPxY * v2->y + n->y, dataPxX * v1->z - dataPxY * v2->z + n->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx, state->viewerState->vpConfigs[currentVP].texture.texRLy);

                        glVertex3f(dataPxX * v1->x + dataPxY * v2->x + n->x, dataPxX * v1->y + dataPxY * v2->y + n->y, dataPxX * v1->z + dataPxY * v2->z + n->z);
                        glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx, state->viewerState->vpConfigs[currentVP].texture.texLLy);

                        glVertex3f(-dataPxX * v1->x + dataPxY * v2->x + n->x, -dataPxX * v1->y + dataPxY * v2->y + n->y, -dataPxX * v1->z + dataPxY * v2->z + n->z);
                    glEnd();

                    // Draw the overlay textures
                    if(state->overlay) {
                        if(state->viewerState->overlayVisible) {
                            glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[currentVP].texture.overlayHandle);
                            glBegin(GL_QUADS);
                                glNormal3i(n->x, n->y, n->z);
                                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLUx,
                                             state->viewerState->vpConfigs[currentVP].texture.texLUy);
                                glVertex3f(-dataPxX * v1->x - dataPxY * v2->x - 0.1 * n->x,

                                           -dataPxX * v1->y - dataPxY * v2->y - 0.1 * n->y,

                                           -dataPxX * v1->z - dataPxY * v2->z - 0.1 * n->z);
                                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRUx,
                                             state->viewerState->vpConfigs[currentVP].texture.texRUy);

                                glVertex3f(dataPxX * v1->x - dataPxY * v2->x - 0.1 * n->x,

                                           dataPxX * v1->y - dataPxY * v2->y - 0.1 * n->y,

                                           dataPxX * v1->z - dataPxY * v2->z - 0.1 * n->z);
                                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texRLx,
                                             state->viewerState->vpConfigs[currentVP].texture.texRLy);

                                glVertex3f(dataPxX * v1->x + dataPxY * v2->x - 0.1 * n->x,

                                           dataPxX * v1->y + dataPxY * v2->y - 0.1 * n->y,

                                           dataPxX * v1->z + dataPxY * v2->z - 0.1 * n->z);
                                glTexCoord2f(state->viewerState->vpConfigs[currentVP].texture.texLLx,
                                             state->viewerState->vpConfigs[currentVP].texture.texLLy);

                                glVertex3f(-dataPxX * v1->x + dataPxY * v2->x - 0.1 * n->x,

                                           -dataPxX * v1->y + dataPxY * v2->y - 0.1 * n->y,

                                           -dataPxX * v1->z + dataPxY * v2->z - 0.1 * n->z);
                            glEnd();
                        }
                    }

                    glBindTexture(GL_TEXTURE_2D, 0);
                    glDisable(GL_DEPTH_TEST);

                    if(state->viewerState->drawVPCrosshairs) {
                        glLineWidth(1.);
                        glBegin(GL_LINES);
                            glColor4f(v2->z, v2->y, v2->x, 0.3);

                            glVertex3f(-dataPxX * v1->x + 0.5 * v2->x - 0.0001 * n->x,

                                       -dataPxX * v1->y + 0.5 * v2->y - 0.0001 * n->y,

                                       -dataPxX * v1->z + 0.5 * v2->z - 0.0001 * n->z);

                            glVertex3f(dataPxX * v1->x + 0.5 * v2->x - 0.0001 * n->x,

                                       dataPxX * v1->y + 0.5 * v2->y - 0.0001 * n->y,

                                       dataPxX * v1->z + 0.5 * v2->z - 0.0001 * n->z);

                            glColor4f(v1->z, v1->y, v1->x, 0.3);

                            glVertex3f(0.5 * v1->x - dataPxY * v2->x - 0.0001 * n->x,

                                       0.5 * v1->y - dataPxY * v2->y - 0.0001 * n->y,

                                       0.5 * v1->z - dataPxY * v2->z - 0.0001 * n->z);

                            glVertex3f(0.5 * v1->x + dataPxY * v2->x - 0.0001 * n->x,

                                       0.5 * v1->y + dataPxY * v2->y - 0.0001 * n->y,

                                       0.5 * v1->z + dataPxY * v2->z - 0.0001 * n->z);
                        glEnd();
                    }
                    /**/
                    break;


    }

    glDisable(GL_BLEND);
    renderViewportBorders(currentVP);

    return true;
}

bool Renderer::renderSkeletonVP(uint currentVP) {
    char *textBuffer;
    char *c;
    uint i;

    GLUquadricObj *gluCylObj = NULL;

    // Used for calculation of slice pane length inside the 3d view
    float dataPxX, dataPxY;

    textBuffer = (char*)malloc(32);
    memset(textBuffer, '\0', 32);

    //glClear(GL_DEPTH_BUFFER_BIT); // better place? TDitem

    if(!state->viewerState->selectModeFlag) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
    }


    // left, right, bottom, top, near, far clipping planes; substitute arbitrary vals to something more sensible. TDitem
//LOG("%f, %f, %f", state->skeletonState->translateX, state->skeletonState->translateY, state->skeletonState->zoomLevel)
    glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
        state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
        state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
        state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY, -1000, 10 *state->skeletonState->volBoundary);

    state->viewerState->vpConfigs[VIEWPORT_SKELETON].screenPxXPerDataPx =
            (float)state->viewerState->vpConfigs[VIEWPORT_SKELETON].edgeLength / ((
            (float)state->skeletonState->volBoundary - 2.f * ((float)state->skeletonState->volBoundary * state->skeletonState->zoomLevel))/2.f);


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
    if(state->viewerState->multisamplingOnOff) glEnable(GL_MULTISAMPLE);


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

        glColor4f(0.9, 0.9, 0.9, 1.); // HERE
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
        if((state->skeletonState->rotdx)
            || (state->skeletonState->rotdy)
            ){


            if((state->skeletonState->rotateAroundActiveNode) && (state->skeletonState->activeNode)) {
                glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2),-((float)state->boundary.z / 2.));
                glTranslatef((float)state->skeletonState->activeNode->position.x,
                             (float)state->skeletonState->activeNode->position.y,
                             (float)state->skeletonState->activeNode->position.z);
                glScalef(1., 1., state->viewerState->voxelXYtoZRatio);
                rotateSkeletonViewport();
                glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
                glTranslatef(-(float)state->skeletonState->activeNode->position.x,
                             -(float)state->skeletonState->activeNode->position.y,
                             -(float)state->skeletonState->activeNode->position.z);
                glTranslatef(((float)state->boundary.x / 2.),((float)state->boundary.y / 2.),((float)state->boundary.z / 2.));
            }
            // rotate around dataset center if no active node is selected
            else {
                glScalef(1., 1., state->viewerState->voxelXYtoZRatio);
                rotateSkeletonViewport();
                glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
            }

            // save the modified basic model view matrix

            glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

            // reset the relative rotation angles because rotation has been performed.
        }

        switch(state->skeletonState->definedSkeletonVpView) {
            case 0:
                break;
            case 1:
                // XY viewport like view
                state->skeletonState->definedSkeletonVpView = 0;

                glLoadIdentity();
                glTranslatef((float)state->skeletonState->volBoundary / 2.,
                             (float)state->skeletonState->volBoundary / 2.,
                             (float)state->skeletonState->volBoundary / -2.);
//glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
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

            case 2:
                // XZ viewport like view
                state->skeletonState->definedSkeletonVpView = 0;

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
                setRotationState(ROTATIONSTATEXZ);
                break;

            case 3:
                // YZ viewport like view
                state->skeletonState->definedSkeletonVpView = 0;
                glLoadIdentity();
                glTranslatef((float)state->skeletonState->volBoundary / 2.,
                             (float)state->skeletonState->volBoundary / 2.,
                             (float)state->skeletonState->volBoundary / -2.);
                glRotatef(270, 1., 0., 0.);
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
                setRotationState(ROTATIONSTATEYZ);
                break;
            //float minrotation[16];
            //float da;
            case 4:
            //90deg
                state->skeletonState->rotdx = 10;
                state->skeletonState->rotationcounter++;
                if (state->skeletonState->rotationcounter > 15) {
                    state->skeletonState->rotdx = 7.6;
                    state->skeletonState->definedSkeletonVpView = 0;
                    state->skeletonState->rotationcounter = 0;
                }

                break;

            case 5:
            //180deg
                state->skeletonState->rotdx = 10;
                state->skeletonState->rotationcounter++;
                if (state->skeletonState->rotationcounter > 31) {
                    state->skeletonState->rotdx = 5.2;
                    state->skeletonState->definedSkeletonVpView = 0;
                    state->skeletonState->rotationcounter = 0;
                }

                break;
            case 6:
                // Resetting
                state->skeletonState->definedSkeletonVpView = 0;
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
        }


         // Draw the slice planes for orientation inside the data stack

        glPushMatrix();

        // single operation! TDitem
        glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
        glTranslatef(0.5,0.5,0.5);

        updateFrustumClippingPlanes(VIEWPORT_SKELETON);
        glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);


        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor4f(1., 1., 1., 1.);

        for(i = 0; i < state->viewerState->numberViewports; i++) {
            dataPxX = state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                    / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                    * 0.5;
            dataPxY = state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                * 0.5;


            switch(state->viewerState->vpConfigs[i].type) {
            case VIEWPORT_XY:
                if(!state->skeletonState->showXYplane) break;


                glEnable(GL_TEXTURE_2D);
                //glDisable(GL_DEPTH_TEST);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glColor4f(1., 1., 1., 1.);


                glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);

                //LOG("skeleton VP tex XY id: %d", state->viewerState->vpConfigs[i].texture.texHandle)
                glLoadName(VIEWPORT_XY);
                glBegin(GL_QUADS);
                    glNormal3i(0,0,1);

                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLUx, state->viewerState->vpConfigs[i].texture.texLUy);
                    glVertex3f(-dataPxX, -dataPxY, 0.);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRUx, state->viewerState->vpConfigs[i].texture.texRUy);
                    glVertex3f(dataPxX, -dataPxY, 0.);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRLx, state->viewerState->vpConfigs[i].texture.texRLy);
                    glVertex3f(dataPxX, dataPxY, 0.);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLLx, state->viewerState->vpConfigs[i].texture.texLLy);
                    glVertex3f(-dataPxX, dataPxY, 0.);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                break;
            case VIEWPORT_XZ:
                if(!state->skeletonState->showXZplane) break;
                glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
                glLoadName(VIEWPORT_XZ);
                glBegin(GL_QUADS);
                    glNormal3i(0,1,0);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLUx, state->viewerState->vpConfigs[i].texture.texLUy);
                    glVertex3f(-dataPxX, 0., -dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRUx, state->viewerState->vpConfigs[i].texture.texRUy);
                    glVertex3f(dataPxX, 0., -dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRLx, state->viewerState->vpConfigs[i].texture.texRLy);
                    glVertex3f(dataPxX, 0., dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLLx, state->viewerState->vpConfigs[i].texture.texLLy);
                    glVertex3f(-dataPxX, 0., dataPxY);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                break;
            case VIEWPORT_YZ:
                if(!state->skeletonState->showYZplane) break;
                glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
                glLoadName(VIEWPORT_YZ);
                glBegin(GL_QUADS);
                    glNormal3i(1,0,0);                    

                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLUx, state->viewerState->vpConfigs[i].texture.texLUy);
                    glVertex3f(0., -dataPxX, -dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRUx, state->viewerState->vpConfigs[i].texture.texRUy);
                    glVertex3f(0., dataPxX, -dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRLx, state->viewerState->vpConfigs[i].texture.texRLy);
                    glVertex3f(0., dataPxX, dataPxY);
                    glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLLx, state->viewerState->vpConfigs[i].texture.texLLy);
                    glVertex3f(0., -dataPxX, dataPxY);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);                
                break;
            case VIEWPORT_ARBITRARY:
                /* @arb */
                floatCoordinate *n, *v1, *v2;
                n = &(state->viewerState->vpConfigs[i].n);

                v1 = &(state->viewerState->vpConfigs[i].v1);

                v2 = &(state->viewerState->vpConfigs[i].v2);

                //TODO, adjust this for arbitrary planes

                //if(!state->skeletonState->showYZplane) break;
                glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
                glLoadName(i);
                glBegin(GL_QUADS);
                glNormal3i(n->x, n->y, n->z);
                glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLUx, state->viewerState->vpConfigs[i].texture.texLUy);
                glVertex3f(-dataPxX * v1->x - dataPxY * v2->x, -dataPxX * v1->y - dataPxY * v2->y, -dataPxX * v1->z - dataPxY * v2->z);
                glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRUx, state->viewerState->vpConfigs[i].texture.texRUy);
                glVertex3f(dataPxX * v1->x - dataPxY * v2->x, dataPxX * v1->y - dataPxY * v2->y, dataPxX * v1->z - dataPxY * v2->z);
                glTexCoord2f(state->viewerState->vpConfigs[i].texture.texRLx, state->viewerState->vpConfigs[i].texture.texRLy);

                glVertex3f(dataPxX * v1->x + dataPxY * v2->x, dataPxX * v1->y + dataPxY * v2->y, dataPxX * v1->z + dataPxY * v2->z);
                glTexCoord2f(state->viewerState->vpConfigs[i].texture.texLLx, state->viewerState->vpConfigs[i].texture.texLLy);

                glVertex3f(-dataPxX * v1->x + dataPxY * v2->x, -dataPxX * v1->y + dataPxY * v2->y, -dataPxX * v1->z + dataPxY * v2->z);
                glEnd();
                glBindTexture (GL_TEXTURE_2D, 0);
                /* */

                break;
            }

        }

        glDisable(GL_TEXTURE_2D);

        for(i = 0; i < state->viewerState->numberViewports; i++) {
            dataPxX = state->viewerState->vpConfigs[i].texture.displayedEdgeLengthX
                / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                * 0.5;
            dataPxY = state->viewerState->vpConfigs[i].texture.displayedEdgeLengthY
                / state->viewerState->vpConfigs[i].texture.texUnitsPerDataPx
                * 0.5;
            switch(state->viewerState->vpConfigs[i].type) {
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
                /* @arb */
                floatCoordinate *n, *v1, *v2;
                n = &(state->viewerState->vpConfigs[i].n);

                                v1 = &(state->viewerState->vpConfigs[i].v1);

                                v2 = &(state->viewerState->vpConfigs[i].v2);

                                glColor4f(n->z, n->y, n->x, 1.);

                                glBegin(GL_LINE_LOOP);
                                    glVertex3f(-dataPxX * v1->x - dataPxY * v2->x, -dataPxX * v1->y - dataPxY * v2->y, -dataPxX * v1->z - dataPxY * v2->z);
                                    glVertex3f(dataPxX * v1->x - dataPxY * v2->x, dataPxX * v1->y - dataPxY * v2->y, dataPxX * v1->z - dataPxY * v2->z);

                                    glVertex3f(dataPxX * v1->x + dataPxY * v2->x, dataPxX * v1->y + dataPxY * v2->y, dataPxX * v1->z + dataPxY * v2->z);

                                    glVertex3f(-dataPxX * v1->x + dataPxY * v2->x, -dataPxX * v1->y + dataPxY * v2->y, -dataPxX * v1->z + dataPxY * v2->z);
                                glEnd();

                //                if (i==0){

                //                    glColor4f(0., 0., 0., 1.);
                //                    glPushMatrix();
                //                    glTranslatef(-dataPxX, 0., 0.);
                //                    glRotatef(90., 0., 1., 0.);
                //                    gluCylObj = gluNewQuadric();
                //                    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                //                    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                //                    gluCylinder(gluCylObj, 0.4, 0.4, dataPxX * 2, 5, 5);
                //                    gluDeleteQuadric(gluCylObj);
                //                    glPopMatrix();
                //
                //                    glPushMatrix();
                //                    glTranslatef(0., dataPxY, 0.);
                //                    glRotatef(90., 1., 0., 0.);
                //                    gluCylObj = gluNewQuadric();
                //                    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                //                    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                //                    gluCylinder(gluCylObj, 0.4, 0.4, dataPxY * 2, 5, 5);
                //                    gluDeleteQuadric(gluCylObj);
                //                    glPopMatrix();

                //                }

                //                else if (i==1){

                //                    glColor4f(0., 0., 0., 1.);
                //                    glPushMatrix();
                //                    glTranslatef(0., 0., -dataPxY);
                //                    gluCylObj = gluNewQuadric();
                //                    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                //                    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
                //                    gluCylinder(gluCylObj, 0.4, 0.4, dataPxY * 2, 5, 5);
                //                    gluDeleteQuadric(gluCylObj);
                //                    glPopMatrix();

                //                }

                /**/
                break;
            }
        }

        glPopMatrix();
        glEnable(GL_TEXTURE_2D);


     // Now we draw the dataset corresponding stuff (volume box of right size, axis descriptions...)
    glEnable(GL_BLEND);

    // Now we draw the data volume box.

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

    glColor4f(0., 0., 0., 1.);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glPushMatrix();
    /*
    glLineWidth(2.f);
    glBegin(GL_LINES);
        glVertex3i(-(state->boundary.x / 2), -(state->boundary.y / 2), -(state->boundary.z / 2));
        glVertex3i((state->boundary.x / 2), -(state->boundary.y / 2), -(state->boundary.z / 2));
    glEnd();
    glLineWidth(1.f);
*/
    glTranslatef(-(state->boundary.x / 2),-(state->boundary.y / 2),-(state->boundary.z / 2));
    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    gluCylinder(gluCylObj, 5., 5. , state->boundary.z, 5, 5);
    gluDeleteQuadric(gluCylObj);

    glPushMatrix();
    glTranslatef(0.,0., state->boundary.z);
    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    gluCylinder(gluCylObj, 15., 0. , 50., 15, 15);
    gluDeleteQuadric(gluCylObj);
    glPopMatrix();

    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    glRotatef(90., 0., 1., 0.);
    gluCylinder(gluCylObj, 5., 5. , state->boundary.x, 5, 5);
    gluDeleteQuadric(gluCylObj);

    glPushMatrix();
    glTranslatef(0.,0., state->boundary.x);
    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    gluCylinder(gluCylObj, 15., 0. , 50., 15, 15);
    gluDeleteQuadric(gluCylObj);
    glPopMatrix();

    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    glRotatef(-90., 1., 0., 0.);
    gluCylinder(gluCylObj, 5., 5. , state->boundary.y, 5, 5);
    gluDeleteQuadric(gluCylObj);

    glPushMatrix();
    glTranslatef(0.,0., state->boundary.y);
    gluCylObj = gluNewQuadric();
    gluQuadricNormals(gluCylObj, GLU_SMOOTH);
    gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
    gluCylinder(gluCylObj, 15., 0. , 50., 15, 15);
    gluDeleteQuadric(gluCylObj);
    glPopMatrix();
    glPopMatrix();

    /* new position */
    renderSkeleton(VIEWPORT_SKELETON);

    Coordinate pos;

    // Draw axis description
    glColor4f(0., 0., 0., 1.);
    memset(textBuffer, '\0', 32);
    glRasterPos3f((float)-(state->boundary.x) / 2. - 50., (float)-(state->boundary.y) / 2. - 50., (float)-(state->boundary.z) / 2. - 50.);

    sprintf(textBuffer, "1, 1, 1");

    pos.x = (float)-(state->boundary.x) / 2. - 50.;
    pos.y = (float)-(state->boundary.y) / 2. - 50.;
    pos.z = (float)-(state->boundary.z) / 2. - 50.;

    renderText(&pos, textBuffer, VIEWPORT_SKELETON);

    memset(textBuffer, '\0', 32);
    glRasterPos3f((float)(state->boundary.x) / 2. - 50., -(state->boundary.y / 2) - 50., -(state->boundary.z / 2)- 50.);
    sprintf(textBuffer, "%d, 1, 1", state->boundary.x + 1);
    pos.x = (float)(state->boundary.x) / 2. - 50., -(state->boundary.y / 2) - 50.;
    pos.y = (float)-(state->boundary.y / 2) - 50.;
    pos.z = (float)-(state->boundary.z) / 2. - 50.;
    renderText(&pos, textBuffer, VIEWPORT_SKELETON);

    memset(textBuffer, '\0', 32);
    glRasterPos3f(-(state->boundary.x / 2)- 50., (float)(state->boundary.y) / 2. - 50., -(state->boundary.z / 2)- 50.);
    sprintf(textBuffer, "1, %d, 1", state->boundary.y + 1);

    pos.x = -(state->boundary.x / 2)- 50.;
    pos.y = (float)(state->boundary.y) / 2. - 50.;
    pos.z = -(state->boundary.z / 2)- 50.;

    renderText(&pos, textBuffer, VIEWPORT_SKELETON);

    memset(textBuffer, '\0', 32);
    glRasterPos3f(-(state->boundary.x / 2)- 50., -(state->boundary.y / 2)- 50., (float)(state->boundary.z) / 2. - 50.);
    sprintf(textBuffer, "1, 1, %d", state->boundary.z + 1);

    pos.x = -(state->boundary.x / 2)- 50.;
    pos.y = -(state->boundary.y / 2)- 50.;
    pos.z = (float)(state->boundary.z) / 2. - 50.;

    renderText(&pos, textBuffer, VIEWPORT_SKELETON);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

     // Reset previously changed OGL parameters

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glLoadIdentity();

    free(textBuffer);

    renderViewportBorders(currentVP);

    return true;
}

#include "sleeper.h"
uint Renderer::retrieveVisibleObjectBeneathSquare(uint currentVP, uint x, uint y, uint width) {

    int i;
    /* 8192 is really arbitrary. It should be a value dependent on the
    number of nodes / segments */


    GLuint selectionBuffer[8192] = {0};
    GLint hits, openGLviewport[4];
    GLuint names, *ptr, minZ, *ptrName;
    ptrName = NULL;

    if(currentVP == VIEWPORT_XY) {
        refVPXY->makeCurrent();
        glGetIntegerv(GL_VIEWPORT, openGLviewport);
    } else if(currentVP == VIEWPORT_XZ) {
        refVPXZ->makeCurrent();
        glGetIntegerv(GL_VIEWPORT, openGLviewport);
    } else if(currentVP == VIEWPORT_YZ) {
        refVPYZ->makeCurrent();
        glGetIntegerv(GL_VIEWPORT, openGLviewport);
    } else if(currentVP == VIEWPORT_SKELETON) {
        refVPSkel->makeCurrent();
        glGetIntegerv(GL_VIEWPORT, openGLviewport);
    }

   // glGetIntegerv(GL_VIEWPORT, openGLviewport);

    glSelectBuffer(8192, selectionBuffer);

    state->viewerState->selectModeFlag = true;

    glRenderMode(GL_SELECT);

    glInitNames();
    glPushName(0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();


    gluPickMatrix(x, refVPXY->height() - y, width, width, openGLviewport);




    if(state->viewerState->vpConfigs[currentVP].type == VIEWPORT_SKELETON) {
        renderSkeletonVP(currentVP);
    } else {
        glDisable(GL_DEPTH_TEST);
        renderOrthogonalVP(currentVP);
    }

    hits = glRenderMode(GL_RENDER);
    qDebug() << hits << " collision";
    glLoadIdentity();

    ptr = (GLuint *)selectionBuffer;

    minZ = 0xffffffff;

    for(int i = 49; i < 100; i++) {
        if(selectionBuffer[i] != 0) {
            qDebug() << i << " " << selectionBuffer[i];
        }

    }



    for(i = 0; i < hits; i++) {
        names = *ptr;

        ptr++;
        if((*ptr < minZ) && (*(ptr + 2) >= 50)) {
            minZ = *ptr;
            ptrName = ptr + 2;
        }
        ptr += names + 2;
    }



    state->viewerState->selectModeFlag = false;
    if(ptrName) {
        return *ptrName - 50;
    } else {
        return false;
    }       
}

bool Renderer::updateRotationStateMatrix(float M1[16], float M2[16]){
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

bool Renderer::rotateSkeletonViewport(){

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


bool Renderer::setRotationState(uint setTo) {
    if (setTo == 0){
            //Reset Viewport
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
        if (setTo == 1){
            //XY view
            state->skeletonState->rotationState[0] = 1.0;
            state->skeletonState->rotationState[1] = 0.0;
            state->skeletonState->rotationState[2] = 0.0;
            state->skeletonState->rotationState[3] = 0.0;
            state->skeletonState->rotationState[4] = 0.0;
            state->skeletonState->rotationState[5] = 1.0;
            state->skeletonState->rotationState[6] = 0.0;
            state->skeletonState->rotationState[7] = 0.0;
            state->skeletonState->rotationState[8] = 0.0;
            state->skeletonState->rotationState[9] = 0.0;
            state->skeletonState->rotationState[10] = 1.0;
            state->skeletonState->rotationState[11] = 0.0;
            state->skeletonState->rotationState[12] = 0.0;
            state->skeletonState->rotationState[13] = 0.0;
            state->skeletonState->rotationState[14] = 0.0;
            state->skeletonState->rotationState[15] = 1.0;
        }
        if (setTo == 2){
            //YZ view
            state->skeletonState->rotationState[0] = 1.0;
            state->skeletonState->rotationState[1] = 0.0;
            state->skeletonState->rotationState[2] = 0.0;
            state->skeletonState->rotationState[3] = 0.0;
            state->skeletonState->rotationState[4] = 0.0;
            state->skeletonState->rotationState[5] = 0.0;
            state->skeletonState->rotationState[6] = -1.0;
            state->skeletonState->rotationState[7] = 0.0;
            state->skeletonState->rotationState[8] = 0.0;
            state->skeletonState->rotationState[9] = 1.0;
            state->skeletonState->rotationState[10] = 0.0;
            state->skeletonState->rotationState[11] = 0.0;
            state->skeletonState->rotationState[12] = 0.0;
            state->skeletonState->rotationState[13] = 0.0;
            state->skeletonState->rotationState[14] = 0.0;
            state->skeletonState->rotationState[15] = 1.0;
        }
        if (setTo == 3){
            //XZ view
            state->skeletonState->rotationState[0] = 0.0;
            state->skeletonState->rotationState[1] = 0.0;
            state->skeletonState->rotationState[2] = -1.0;
            state->skeletonState->rotationState[3] = 0.0;
            state->skeletonState->rotationState[4] = 0.0;
            state->skeletonState->rotationState[5] = -1.0;
            state->skeletonState->rotationState[6] = 0.0;
            state->skeletonState->rotationState[7] = 0.0;
            state->skeletonState->rotationState[8] = -1.0;
            state->skeletonState->rotationState[9] = 0.0;
            state->skeletonState->rotationState[10] = 0.0;
            state->skeletonState->rotationState[11] = 0.0;
            state->skeletonState->rotationState[12] = 0.0;
            state->skeletonState->rotationState[13] = 0.0;
            state->skeletonState->rotationState[14] = 0.0;
            state->skeletonState->rotationState[15] = 1.0;
        }
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

void Renderer::renderSkeleton(uint viewportType) {
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode, *lastNode = NULL, *lastRenderedNode = NULL;
    struct segmentListElement *currentSegment;
    float cumDistToLastRenderedNode;
    floatCoordinate currNodePos;
    uint virtualSegRendered, allowHeuristic;
    uint skippedCnt = 0;
    uint renderNode;
    float currentRadius;


    state->skeletonState->lineVertBuffer.vertsIndex = 0;
    state->skeletonState->lineVertBuffer.normsIndex = 0;
    state->skeletonState->lineVertBuffer.colsIndex = 0;

    state->skeletonState->pointVertBuffer.vertsIndex = 0;
    state->skeletonState->pointVertBuffer.normsIndex = 0;
    state->skeletonState->pointVertBuffer.colsIndex = 0;
    color4F currentColor;

    char *textBuffer;
    textBuffer = (char *)malloc(32);
    memset(textBuffer, '\0', 32);

    if((state->skeletonState->displayMode & DSP_SLICE_VP_HIDE)) {
        if(viewportType != VIEWPORT_SKELETON) {
            return;
        }
    }

    if((state->skeletonState->displayMode & DSP_SKEL_VP_HIDE)) {
        if(viewportType == VIEWPORT_SKELETON) {
            return;
        }
    }

    //tdItem: test culling under different conditions!
    //if(viewportType == VIEWPORT_SKELETON) glEnable(GL_CULL_FACE);

    /* Enable blending just once, since we never disable it? */
    glEnable(GL_BLEND);

    glPushMatrix();

    /* Rendering of objects starts always at the origin of our data pixel
    coordinate system. Thus, we have to translate there. */
    glTranslatef(-(float)state->boundary.x / 2. + 0.5,-(float)state->boundary.y / 2. + 0.5,-(float)state->boundary.z / 2. + 0.5);

    /* We iterate over the whole tree structure. */
    currentTree = state->skeletonState->firstTree;

    while(currentTree) {

        lastNode = NULL;
        lastRenderedNode = NULL;
        cumDistToLastRenderedNode = 0.f;

        if(state->skeletonState->displayMode & DSP_ACTIVETREE) {
            if(currentTree != state->skeletonState->activeTree) {
                currentTree = currentTree->next;
                continue;
            }
        }

        currentNode = currentTree->firstNode;
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
            if(!sphereInFrustum(currNodePos, currentNode->circRadius, viewportType)) {
                currentNode = currentNode->next;
                lastNode = lastRenderedNode = NULL;
                //skippedCnt++;
                continue;
            }

            virtualSegRendered = false;
            renderNode = true;

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
            while(currentSegment && allowHeuristic) {
                /* isBranchNode tells you only whether the node is on the branch point stack,
                 * not whether it is actually a node connected to more than two other nodes! */
                if((currentSegment->target == lastNode)
                    || ((currentSegment->source == lastNode)
                    &&
                    (!(
                       (currentNode->comment)
                       || (currentNode->isBranchNode)
                       || (currentNode->numSegs > 2)
                       || (currentNode->radius * state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx > 5.f))))) {

                    /* Node is a candidate for LOD culling */

                    /* Do we really skip this node? Test cum dist. to last rendered node! */
                    cumDistToLastRenderedNode += currentSegment->length
                        * state->viewerState->vpConfigs[viewportType].screenPxXPerDataPx;

                    if(cumDistToLastRenderedNode > state->viewerState->cumDistRenderThres) {
                        break;
                    }
                    else {
                        renderNode = false;
                        break;
                    }
                }
                currentSegment = currentSegment->next;
            }

            if(renderNode) {

                /* This sets the current color for the segment rendering */
                if((currentTree->treeID == state->skeletonState->activeTree->treeID)
                    && (state->skeletonState->highlightActiveTree)) {
                        SET_COLOR(currentColor, 1.f, 0.f, 0.f, 1.f);
                }
                else {
                    currentColor = currentTree->color;
                }

                cumDistToLastRenderedNode = 0.f;

                if(lastNode != lastRenderedNode) {
                    virtualSegRendered = true;
                    /* We need a "virtual" segment now */

                    if(state->viewerState->selectModeFlag)
                        glLoadName(3);

                    renderCylinder(&(lastRenderedNode->position),
                                   lastRenderedNode->radius * state->skeletonState->segRadiusToNodeRadius,
                                   &(currentNode->position),
                                   currentNode->radius * state->skeletonState->segRadiusToNodeRadius,
                                   currentColor,
                                   viewportType);
                }

                /* Second pass over segments needed... But only if node is actually rendered! */
                currentSegment = currentNode->firstSegment;
                while(currentSegment) {

                    //2 indicates a backward connection, which should not be rendered.
                    if(currentSegment->flag == 2){
                        currentSegment = currentSegment->next;
                        continue;
                    }

                    if(virtualSegRendered
                       && ((currentSegment->source == lastNode)
                       || (currentSegment->target == lastNode))) {
                        currentSegment = currentSegment->next;
                        continue;
                    }

                    if(state->viewerState->selectModeFlag)
                        glLoadName(3);

                    if(state->skeletonState->overrideNodeRadiusBool)
                        renderCylinder(&(currentSegment->source->position),
                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius,
                            &(currentSegment->target->position),
                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius,
                            currentColor,
                            viewportType);
                    else
                        renderCylinder(&(currentSegment->source->position),
                            currentSegment->source->radius * state->skeletonState->segRadiusToNodeRadius,
                            &(currentSegment->target->position),
                            currentSegment->target->radius * state->skeletonState->segRadiusToNodeRadius,
                            currentColor,
                            viewportType);

                    if(viewportType != VIEWPORT_SKELETON) {
                        if(state->skeletonState->showIntersections)
                            renderSegPlaneIntersection(currentSegment);
                    }

                    currentSegment = currentSegment->next;

                }

                /* The first 50 entries of the openGL namespace are reserved
                for static objects (like slice plane quads...) */
                if(state->viewerState->selectModeFlag)
                    glLoadName(currentNode->nodeID + 50);

                /* Changes the current color & radius if the node has a comment */
                /* This is a bit hackish, but does the job */
                Skeletonizer::setColorFromNode(currentNode, &currentColor);
                Skeletonizer::setRadiusFromNode(currentNode, &currentRadius);

                renderSphere(&(currentNode->position), currentRadius, currentColor, viewportType);

                /* Render the node description only when option is set. */
                if(state->skeletonState->showNodeIDs) {
                    glColor4f(0.f, 0.f, 0.f, 1.f);
                    memset(textBuffer, '\0', 32);
                    sprintf(textBuffer, "%d", currentNode->nodeID);
                    renderText(&(currentNode->position), textBuffer, viewportType);





                }
                lastRenderedNode = currentNode;
            }
            //else skippedCnt++;

            lastNode = currentNode;

            currentNode = currentNode->next;
        }

        currentTree = currentTree->next;
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

    if(state->skeletonState->overrideNodeRadiusBool)
        glPointSize(state->skeletonState->overrideNodeRadiusVal);
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

    //LOG("verts lines: %d", state->skeletonState->lineVertBuffer.vertsIndex)
    //LOG("verts points: %d", state->skeletonState->pointVertBuffer.vertsIndex)

    /* Highlight active node */
    if(state->skeletonState->activeNode) {

        /* Set the default color for the active node */
        SET_COLOR(currentColor, 1.f, 0.f, 0.f, 0.2f);

        /* Color gets changes in case there is a comment & conditional comment
        highlighting */
        Skeletonizer::setColorFromNode(state->skeletonState->activeNode, &currentColor);
        currentColor.a = 0.2f;

        if(state->viewerState->selectModeFlag)
            glLoadName(state->skeletonState->activeNode->nodeID + 50);

        if(state->skeletonState->overrideNodeRadiusBool)
            renderSphere(&(state->skeletonState->activeNode->position), state->skeletonState->overrideNodeRadiusVal * 1.5, currentColor, viewportType);
        else
            renderSphere(&(state->skeletonState->activeNode->position), state->skeletonState->activeNode->radius * 1.5, currentColor, viewportType);

        /* Description of active node is always rendered,
        ignoring state->skeletonState->showNodeIDs */

        glColor4f(0., 0., 0., 1.);
        memset(textBuffer, '\0', 32);
        sprintf(textBuffer, "%d", state->skeletonState->activeNode->nodeID);
        renderText(&(state->skeletonState->activeNode->position), textBuffer, viewportType);


    }
    /* Restore modelview matrix */
    glPopMatrix();
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    free(textBuffer);
    //float tmp = (float)skippedCnt / ((float)state->skeletonState->totalNodeElements +1.f) * 100.f;
    //LOG("percent nodes skipped in this DL: %f", tmp)
    return;

}


bool Renderer::doubleMeshCapacity(mesh *toDouble) {

    (*toDouble).vertices = (floatCoordinate *)realloc((*toDouble).vertices, 2 * (*toDouble).vertsBuffSize * sizeof(floatCoordinate));
    (*toDouble).normals = (floatCoordinate *)realloc((*toDouble).normals, 2 * (*toDouble).normsBuffSize * sizeof(floatCoordinate));
    (*toDouble).colors = (color4F *)realloc((*toDouble).colors, 2 * (*toDouble).colsBuffSize * sizeof(color4F));

    (*toDouble).vertsBuffSize = 2 * (*toDouble).vertsBuffSize;
    (*toDouble).normsBuffSize = 2 * (*toDouble).normsBuffSize;
    (*toDouble).colsBuffSize = 2 * (*toDouble).colsBuffSize;


    return true;
}

bool Renderer::initMesh(mesh *toInit, uint initialSize) {


    (*toInit).vertices = (floatCoordinate *)malloc(initialSize * sizeof(floatCoordinate));
    (*toInit).normals = (floatCoordinate *)malloc(initialSize * sizeof(floatCoordinate));
    (*toInit).colors = (color4F *)malloc(initialSize * sizeof(color4F));

    (*toInit).vertsBuffSize = initialSize;
    (*toInit).normsBuffSize = initialSize;
    (*toInit).colsBuffSize = initialSize;

    (*toInit).vertsIndex = 0;
    (*toInit).normsIndex = 0;
    (*toInit).colsIndex = 0;


    return true;
}


bool Renderer::updateFrustumClippingPlanes(uint viewportType) {
   float   frustum[6][4];
   float   proj[16];
   float   modl[16];
   float   clip[16];
   float   t;

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
   frustum[0][0] = clip[ 3] - clip[ 0];
   frustum[0][1] = clip[ 7] - clip[ 4];
   frustum[0][2] = clip[11] - clip[ 8];
   frustum[0][3] = clip[15] - clip[12];

   /* Normalize the result */
   t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
   frustum[0][0] /= t;
   frustum[0][1] /= t;
   frustum[0][2] /= t;
   frustum[0][3] /= t;

   /* Extract the numbers for the LEFT plane */
   frustum[1][0] = clip[ 3] + clip[ 0];
   frustum[1][1] = clip[ 7] + clip[ 4];
   frustum[1][2] = clip[11] + clip[ 8];
   frustum[1][3] = clip[15] + clip[12];

   /* Normalize the result */
   t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
   frustum[1][0] /= t;
   frustum[1][1] /= t;
   frustum[1][2] /= t;
   frustum[1][3] /= t;

   /* Extract the BOTTOM plane */
   frustum[2][0] = clip[ 3] + clip[ 1];
   frustum[2][1] = clip[ 7] + clip[ 5];
   frustum[2][2] = clip[11] + clip[ 9];
   frustum[2][3] = clip[15] + clip[13];

   /* Normalize the result */
   t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
   frustum[2][0] /= t;
   frustum[2][1] /= t;
   frustum[2][2] /= t;
   frustum[2][3] /= t;

   /* Extract the TOP plane */
   frustum[3][0] = clip[ 3] - clip[ 1];
   frustum[3][1] = clip[ 7] - clip[ 5];
   frustum[3][2] = clip[11] - clip[ 9];
   frustum[3][3] = clip[15] - clip[13];

   /* Normalize the result */
   t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
   frustum[3][0] /= t;
   frustum[3][1] /= t;
   frustum[3][2] /= t;
   frustum[3][3] /= t;

   /* Extract the FAR plane */
   frustum[4][0] = clip[ 3] - clip[ 2];
   frustum[4][1] = clip[ 7] - clip[ 6];
   frustum[4][2] = clip[11] - clip[10];
   frustum[4][3] = clip[15] - clip[14];

   /* Normalize the result */
   t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
   frustum[4][0] /= t;
   frustum[4][1] /= t;
   frustum[4][2] /= t;
   frustum[4][3] /= t;

   /* Extract the NEAR plane */
   frustum[5][0] = clip[ 3] + clip[ 2];
   frustum[5][1] = clip[ 7] + clip[ 6];
   frustum[5][2] = clip[11] + clip[10];
   frustum[5][3] = clip[15] + clip[14];

   /* Normalize the result */
   t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
   frustum[5][0] /= t;
   frustum[5][1] /= t;
   frustum[5][2] /= t;
   frustum[5][3] /= t;


   memcpy(state->viewerState->vpConfigs[viewportType].frustum,
          frustum, sizeof(frustum));
    return true;
}

/* modified public domain code from: http://www.crownandcutlass.com/features/technicaldetails/frustum.html */
bool Renderer::sphereInFrustum(floatCoordinate pos, float radius, uint viewportType) {
    int p;

    /* Include more for rendering when in SELECT mode to avoid picking trouble - 900 px is really arbitrary */
    if(state->viewerState->selectModeFlag) radius += 900.f;

    for( p = 0; p < 6; p++ )
        if( state->viewerState->vpConfigs[viewportType].frustum[p][0]
           * pos.x + state->viewerState->vpConfigs[viewportType].frustum[p][1]
           * pos.y + state->viewerState->vpConfigs[viewportType].frustum[p][2]
           * pos.z + state->viewerState->vpConfigs[viewportType].frustum[p][3]
           <= -radius )
           return false;

       return true;
}

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

// refreshtimelabel should be replaced by better code now
// #define GLUT_DISABLE_ATEXIT_HACK what is this?

#include "renderer.h"
#include "viewer.h"
#include <math.h>

#include <QtOpenGL>
#if QT_VERSION < 5
    #ifdef Q_WS_MACX
        #include <OpenGL.h>
        #include <glu.h>
    #else
        #include <GL/gl.h>
        #include <GL/glext.h>
        #include <GL/glu.h>
    #endif
#elif QT_VERSION >= 5
    #ifdef Q_OS_MACX
        #include <OpenGL.h>
        #include <glu.h>
    #else
        #include <GL/gl.h>
        #include <GL/glext.h>
        #include <GL/glu.h>
    #endif
#endif


extern stateInfo *state;
extern stateInfo *tempConfig;


static uint32_t renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius) {
    float currentAngle = 0.;
    //int32_t transFactor = 1;
    //Coordinate transBase, transTop;
    floatCoordinate segDirection, tempVec, *tempVec2;
    GLUquadricObj *gluCylObj = NULL;

    //if(coordinateMag == ORIGINAL_MAG_COORDINATES) {
    //    transFactor = 1.f;
    //}

    /*transBase.x = base->x / transFactor;
    transBase.y = base->y / transFactor;
    transBase.z = base->z / transFactor;
    transTop.x = top->x / transFactor;
    transTop.y = top->y / transFactor;
    transTop.z = top->z / transFactor;
    topRadius = topRadius / transFactor;
    baseRadius = baseRadius / transFactor;
*/
    if(!(state->skeletonState->displayMode & DSP_LINES_POINTS)) {
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
        tempVec2 = Renderer::crossProduct(&tempVec, &segDirection);
        currentAngle = Renderer::radToDeg(Renderer::vectorAngle(&tempVec, &segDirection));

        //some gl implementations have problems with the params occuring for
        //segs in straight directions. we need a fix here.
        glRotatef(currentAngle, tempVec2->x, tempVec2->y, tempVec2->z);

        free(tempVec2);

        gluCylinder(gluCylObj, baseRadius, topRadius, Renderer::euclidicNorm(&segDirection), 4, 1);
        gluDeleteQuadric(gluCylObj);
        glPopMatrix();
    }
    else {
        glBegin(GL_LINES);
            glVertex3f((float)base->x, (float)base->y, (float)base->z);
            glVertex3f((float)top->x, (float)top->y, (float)top->z);
        glEnd();
    }

    return true;
}

static uint32_t renderSphere(Coordinate *pos, float radius) {
    GLUquadricObj *gluSphereObj = NULL;
    //Coordinate transPos;
    int32_t transFactor = 1;

    //if(coordinateMag == ORIGINAL_MAG_COORDINATES)
    //    transFactor = state->magnification;

    //transPos.x = pos->x / transFactor;
    //transPos.y = pos->y / transFactor;
    //transPos.z = pos->z / transFactor;

    radius = radius / (float)transFactor;

    /* Render point instead of sphere if user has chosen mode */
    if(!(state->skeletonState->displayMode & DSP_LINES_POINTS)) {
        glPushMatrix();
        glTranslatef((float)pos->x, (float)pos->y, (float)pos->z);
        gluSphereObj = gluNewQuadric();
        gluQuadricNormals(gluSphereObj, GLU_SMOOTH);
        gluQuadricOrientation(gluSphereObj, GLU_OUTSIDE);

        gluSphere(gluSphereObj, radius, 5, 5);

        gluDeleteQuadric(gluSphereObj);
        glPopMatrix();
    }
    else {
        glPointSize(radius*3.);
        glBegin(GL_POINTS);
            glVertex3f((float)pos->x, (float)pos->y, (float)pos->z);
        glEnd();
        glPointSize(1.);
    }

    return true;
}

static uint32_t renderText(Coordinate *pos, char *string) {

    char *c;
    //int32_t transFactor = 1;
    //Coordinate transPos;

    //if(coordinateMag == ORIGINAL_MAG_COORDINATES)
    //    transFactor = state->magnification;

    //transPos.x = pos->x / transFactor;
    //transPos.y = pos->y / transFactor;
    //transPos.z = pos->z / transFactor;

    glDisable(GL_DEPTH_TEST);
    glRasterPos3d(pos->x, pos->y, pos->z);
    for (c = string; *c != '\0'; c++) {
        //glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c); // TODO Removed due to glut32 problems
    }
    glEnable(GL_DEPTH_TEST);

    return true;
}


static uint32_t renderSegPlaneIntersection(struct segmentListElement *segment) {

    if(!state->skeletonState->showIntersections) return true;
    if(state->skeletonState->displayMode & DSP_LINES_POINTS) return true;

    float p[2][3], a, currentAngle, length, radius, distSourceInter, sSr_local, sTr_local;
    int32_t i, distToCurrPos;
    floatCoordinate *tempVec2, tempVec, tempVec3, segDir, intPoint, sTp_local, sSp_local;
    GLUquadricObj *gluCylObj = NULL;

    sSp_local.x = (float)segment->source->position.x;// / state->magnification;
    sSp_local.y = (float)segment->source->position.y;// / state->magnification;
    sSp_local.z = (float)segment->source->position.z;// / state->magnification;

    sTp_local.x = (float)segment->target->position.x;// / state->magnification;
    sTp_local.y = (float)segment->target->position.y;// / state->magnification;
    sTp_local.z = (float)segment->target->position.z;// / state->magnification;

    sSr_local = (float)segment->source->radius;// / state->magnification;
    sTr_local = (float)segment->target->radius;// / state->magnification;

    //n contains the normal vectors of the 3 orthogonal planes
    float n[3][3] = {{1.,0.,0.},
                    {0.,1.,0.},
                    {0.,0.,1.}};

    distToCurrPos = state->viewerState->zoomCube + 1 * state->cubeEdgeLength;

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
        if(Renderer::sgn(p[0][i])*Renderer::sgn(p[1][i]) == -1) {
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
            if(abs((int32_t)intPoint.x - state->viewerState->currentPosition.x) <= distToCurrPos
                && abs((int32_t)intPoint.y - state->viewerState->currentPosition.y) <= distToCurrPos
                && abs((int32_t)intPoint.z - state->viewerState->currentPosition.z) <= distToCurrPos) {

                //Render a cylinder to highlight the intersection
                glPushMatrix();
                gluCylObj = gluNewQuadric();
                gluQuadricNormals(gluCylObj, GLU_SMOOTH);
                gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);

                length = Renderer::euclidicNorm(&segDir);
                distSourceInter = Renderer::euclidicNorm(&tempVec3);

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
                tempVec2 = Renderer::crossProduct(&tempVec, &segDir);
                currentAngle = Renderer::radToDeg(Renderer::vectorAngle(&tempVec, &segDir));
                glRotatef(currentAngle, tempVec2->x, tempVec2->y, tempVec2->z);
                free(tempVec2);

                glColor4f(0.,0.,0.,1.);

                if(state->skeletonState->overrideNodeRadiusBool)
                    gluCylinder(gluCylObj,
                        state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius*1.2,
                        state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius*1.2,
                        1.5, 4, 1);

                else gluCylinder(gluCylObj,
                        radius * state->skeletonState->segRadiusToNodeRadius*1.2,
                        radius * state->skeletonState->segRadiusToNodeRadius*1.2,
                        1.5, 4, 1);

                gluDeleteQuadric(gluCylObj);
                glPopMatrix();
            }

        }
    }

    return true;

}

static uint32_t renderViewportBorders(uint32_t currentVP) {
    /*
    char *description;
    char *c;

    setOGLforVP(currentVP);


    description = malloc(512);
    memset(description, '\0', 512);

    Draw move button in the upper left corner
    glColor4f(0.5, 0.5, 0.5, 0.7);
    glBegin(GL_QUADS);
    glVertex3i(0, 0, 0);
    glVertex3i(10, 0, 0);
    glVertex3i(10, 10, 0);
    glVertex3i(0, 10, 0);
    glEnd();
    //Draw resize button in the lower right corner
    glBegin(GL_QUADS);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength - 10, state->viewerState->vpConfigs[currentVP].edgeLength - 10, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength, state->viewerState->vpConfigs[currentVP].edgeLength - 10, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength, state->viewerState->vpConfigs[currentVP].edgeLength, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength - 10, state->viewerState->vpConfigs[currentVP].edgeLength, 0);
    glEnd();
    //glTranslatef(0., 0., -0.5);
    //Draw button borders
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor4f(0., 0., 0., 1.);
    glBegin(GL_QUADS);
    glVertex3i(0, 0, 0);
    glVertex3i(10, 0, 0);
    glVertex3i(10, 10, 0);
    glVertex3i(0, 10, 0);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength - 10, state->viewerState->vpConfigs[currentVP].edgeLength - 10, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength, state->viewerState->vpConfigs[currentVP].edgeLength - 10, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength, state->viewerState->vpConfigs[currentVP].edgeLength, 0);
    glVertex3i(state->viewerState->vpConfigs[currentVP].edgeLength - 10, state->viewerState->vpConfigs[currentVP].edgeLength, 0);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
*/

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
    }
    glLineWidth(3.);
    glBegin(GL_LINES);
        glVertex3d(2, 1, 0);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, 1, 0);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, 1, 0);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, state->viewerState->vpConfigs[currentVP].edgeLength - 1, 0);
        glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 1, state->viewerState->vpConfigs[currentVP].edgeLength - 1, 0);
        glVertex3d(2, state->viewerState->vpConfigs[currentVP].edgeLength - 2, 0);
        glVertex3d(2, state->viewerState->vpConfigs[currentVP].edgeLength - 2, 0);
        glVertex3d(2, 1, 0);
    glEnd();

    if(state->viewerState->vpConfigs[currentVP].type == state->viewerState->highlightVp) {
        // Draw an orange border to highlight the viewport.

        glColor4f(1., 0.3, 0., 1.);
        glBegin(GL_LINES);
            glVertex3d(5, 4, 0);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, 4, 0);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, 4, 0);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, state->viewerState->vpConfigs[currentVP].edgeLength - 4, 0);
            glVertex3d(state->viewerState->vpConfigs[currentVP].edgeLength - 4, state->viewerState->vpConfigs[currentVP].edgeLength - 4, 0);
            glVertex3d(5, state->viewerState->vpConfigs[currentVP].edgeLength - 5, 0);
            glVertex3d(5, state->viewerState->vpConfigs[currentVP].edgeLength - 5, 0);
            glVertex3d(5, 4, 0);
        glEnd();
    }

    glLineWidth(1.);

    /*
    //Draw VP description
    //set the drawing area in the window to our actually processed view port.
    glViewport(state->viewerState->vpConfigs[currentVP].lowerLeftCorner.x - 5,
               state->viewerState->vpConfigs[currentVP].lowerLeftCorner.y,
               state->viewerState->vpConfigs[currentVP].edgeLength,
               state->viewerState->vpConfigs[currentVP].edgeLength + 5);
    //select the projection matrix
    glMatrixMode(GL_PROJECTION);
    //reset it
    glLoadIdentity();

    //This is necessary to draw the text to the "outside" of the current VP
    //define coordinate system for our viewport: left right bottom top near far
    //coordinate values
    glOrtho(0, state->viewerState->vpConfigs[currentVP].edgeLength + 5,
            state->viewerState->vpConfigs[currentVP].edgeLength + 5, 0, 25, -25);
    //select the modelview matrix for modification
    glMatrixMode(GL_MODELVIEW);
    //reset it
    glLoadIdentity();

    glColor4f(0., 0., 0., 1.);

    switch(state->viewerState->vpConfigs[currentVP].type) {
    case VIEWPORT_XY:
        glRasterPos2i(9, 0);
        sprintf(description, "Viewport XY     x length: %3.3f[um]    y length: %3.3f[um]", state->viewerState->vpConfigs[currentVP].displayedlengthInNmX / 1000., state->viewerState->vpConfigs[currentVP].displayedlengthInNmY / 1000.);
        for (c=description; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
        }
        break;
    case VIEWPORT_XZ:
        glRasterPos2i(9, 0);
        sprintf(description, "Viewport XZ     x length: %3.3f[um]    y length: %3.3f[um]", state->viewerState->vpConfigs[currentVP].displayedlengthInNmX / 1000., state->viewerState->vpConfigs[currentVP].displayedlengthInNmY / 1000.);
        for (c=description; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
        }
        break;
    case VIEWPORT_YZ:
        glRasterPos2i(9, 0);
        sprintf(description, "Viewport YZ     x length: %3.3f[um]    y length: %3.3f[um]", state->viewerState->vpConfigs[currentVP].displayedlengthInNmX / 1000., state->viewerState->vpConfigs[currentVP].displayedlengthInNmY / 1000.);
        for (c=description; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
        }
        break;

    case VIEWPORT_SKELETON:
        glRasterPos2i(9, 0);
        sprintf(description, "Viewport Skeleton     #trees: %d    #nodes: %d    #segments: %d", state->skeletonState->treeElements, state->skeletonState->totalNodeElements, state->skeletonState->totalSegmentElements);
        for (c=description; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
        }
        break;
    }

    free(description);
    */
    return true;
}

Renderer::Renderer(QObject *parent) :
    QObject(parent)
{
}

bool Renderer::drawGUI() {
     return true;
}

bool Renderer::renderOrthogonalVP(uint32_t currentVP) {
    float dataPxX, dataPxY;

    if(!((state->viewerState->vpConfigs[currentVP].type == VIEWPORT_XY)
            || (state->viewerState->vpConfigs[currentVP].type == VIEWPORT_XZ)
            || (state->viewerState->vpConfigs[currentVP].type == VIEWPORT_YZ))) {
        LOG("Wrong VP type given for renderOrthogonalVP() call.");
        return false;
    }

    /* probably not needed TDitem
    glColor4f(0.5, 0.5, 0.5, 1.);
    glBegin(GL_QUADS);
        glVertex2d(0, 0);
        glVertex2d(state->viewerState->vpConfigs[currentVP].edgeLength, 0);
        glVertex2d(state->viewerState->vpConfigs[currentVP].edgeLength, state->viewerState->vpConfigs[currentVP].edgeLength);
        glVertex2d(0, state->viewerState->vpConfigs[currentVP].edgeLength);
    glEnd();
    */


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

            glTranslatef((float)state->viewerState->currentPosition.x,
                        (float)state->viewerState->currentPosition.y,
                        (float)state->viewerState->currentPosition.z);

            glRotatef(180., 1.,0.,0.);

            glLoadName(3);

            glEnable(GL_TEXTURE_2D);
            glDisable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColor4f(1., 1., 1., 1.);

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

            if(state->skeletonState->displayListSkeletonSlicePlaneVP) glCallList(state->skeletonState->displayListSkeletonSlicePlaneVP);

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);
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

            if(state->skeletonState->displayListSkeletonSlicePlaneVP) glCallList(state->skeletonState->displayListSkeletonSlicePlaneVP);

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);
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

            if(state->skeletonState->displayListSkeletonSlicePlaneVP) glCallList(state->skeletonState->displayListSkeletonSlicePlaneVP);

            glTranslatef(-((float)state->boundary.x / 2.),-((float)state->boundary.y / 2.),-((float)state->boundary.z / 2.));
            glTranslatef((float)state->viewerState->currentPosition.x, (float)state->viewerState->currentPosition.y, (float)state->viewerState->currentPosition.z);
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

            break;
    }

    glDisable(GL_BLEND);
    renderViewportBorders(currentVP);

    return true;
}

bool Renderer::renderSkeletonVP(uint32_t currentVP) {
    char *textBuffer;
        char *c;
        uint32_t i;

        GLUquadricObj *gluCylObj = NULL;

        // Used for calculation of slice pane length inside the 3d view
        float dataPxX, dataPxY;

        textBuffer = (char*)malloc(32);
        memset(textBuffer, '\0', 32);

        glClear(GL_DEPTH_BUFFER_BIT); // better place? TDitem

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        // left, right, bottom, top, near, far clipping planes; substitute arbitrary vals to something more sensible. TDitem
    //LOG("%f, %f, %f", state->skeletonState->translateX, state->skeletonState->translateY, state->skeletonState->zoomLevel);
        glOrtho(state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateX,
            state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateX,
            state->skeletonState->volBoundary - (state->skeletonState->volBoundary * state->skeletonState->zoomLevel) + state->skeletonState->translateY,
            state->skeletonState->volBoundary * state->skeletonState->zoomLevel + state->skeletonState->translateY, -1000, 10 *state->skeletonState->volBoundary);

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

        state->skeletonState->viewChanged = true;
        if(state->skeletonState->viewChanged) {
            state->skeletonState->viewChanged = false;
            if(state->skeletonState->displayListView) glDeleteLists(state->skeletonState->displayListView, 1);
            state->skeletonState->displayListView = glGenLists(1);
            // COMPILE_AND_EXECUTE because we grab the rotation matrix inside!
            glNewList(state->skeletonState->displayListView, GL_COMPILE_AND_EXECUTE);

            glEnable(GL_DEPTH_TEST);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

             // Now we draw the  background of our skeleton VP


            glPushMatrix();
            glTranslatef(0., 0., -10. * ((float)state->skeletonState->volBoundary - 2.));

            glShadeModel(GL_SMOOTH);
            glDisable(GL_TEXTURE_2D);

            glLoadName(1);
            glColor4f(0.9, 0.9, 0.9, 1.);
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

                    glBindTexture(GL_TEXTURE_2D, state->viewerState->vpConfigs[i].texture.texHandle);
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
                }

            }

            glDisable(GL_TEXTURE_2D);
            glLoadName(3);
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
                }
            }

            glPopMatrix();
            glEndList();
        }
        else {
            glCallList(state->skeletonState->displayListView);
        }


         // Now we draw the skeleton structure (Changes of it are adressed inside updateSkeletonDisplayList())

        if(state->skeletonState->displayListSkeletonSkeletonizerVP)
            glCallList(state->skeletonState->displayListSkeletonSkeletonizerVP);


         // Now we draw the dataset corresponding stuff (volume box of right size, axis descriptions...)


        if(state->skeletonState->datasetChanged) {

            state->skeletonState->datasetChanged = false;
            if(state->skeletonState->displayListDataset) glDeleteLists(state->skeletonState->displayListDataset, 1);
            state->skeletonState->displayListDataset = glGenLists(1);
            glNewList(state->skeletonState->displayListDataset, GL_COMPILE);
            glEnable(GL_BLEND);


             // Now we draw the data volume box. use display list for that...very static TDitem


            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLoadName(3);
            glColor4f(0., 0., 0., 0.1);
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
            glTranslatef(-(state->boundary.x / 2),-(state->boundary.y / 2),-(state->boundary.z / 2));
            gluCylObj = gluNewQuadric();
            gluQuadricNormals(gluCylObj, GLU_SMOOTH);
            gluQuadricOrientation(gluCylObj, GLU_OUTSIDE);
            gluCylinder(gluCylObj, 3., 3. , state->boundary.z, 5, 5);
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
            gluCylinder(gluCylObj, 3., 3. , state->boundary.x, 5, 5);
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
            gluCylinder(gluCylObj, 3., 3. , state->boundary.y, 5, 5);
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


             // Draw axis description


            glColor4f(0., 0., 0., 1.);
            memset(textBuffer, '\0', 32);
            glRasterPos3f((float)-(state->boundary.x) / 2. - 50., (float)-(state->boundary.y) / 2. - 50., (float)-(state->boundary.z) / 2. - 50.);
            sprintf(textBuffer, "1, 1, 1");
            for (c=textBuffer; *c != '\0'; c++) {
           //     glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
            }
            memset(textBuffer, '\0', 32);
            glRasterPos3f((float)(state->boundary.x) / 2. - 50., -(state->boundary.y / 2) - 50., -(state->boundary.z / 2)- 50.);
            sprintf(textBuffer, "%d, 1, 1", state->boundary.x + 1);
            for (c=textBuffer; *c != '\0'; c++) {
           //     glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
            }

            memset(textBuffer, '\0', 32);
            glRasterPos3f(-(state->boundary.x / 2)- 50., (float)(state->boundary.y) / 2. - 50., -(state->boundary.z / 2)- 50.);
            sprintf(textBuffer, "1, %d, 1", state->boundary.y + 1);
            for (c=textBuffer; *c != '\0'; c++) {
           //     glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
            }
            memset(textBuffer, '\0', 32);
            glRasterPos3f(-(state->boundary.x / 2)- 50., -(state->boundary.y / 2)- 50., (float)(state->boundary.z) / 2. - 50.);
            sprintf(textBuffer, "1, 1, %d", state->boundary.z + 1);
            for (c=textBuffer; *c != '\0'; c++) {
           //     glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, *c);
            }
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
            glEndList();
            glCallList(state->skeletonState->displayListDataset);

        }
        else {
            glCallList(state->skeletonState->displayListDataset);
        }


         // Reset previously changed OGL parameters


        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        glDisable(GL_MULTISAMPLE);
        glLoadIdentity();

        free(textBuffer);

        renderViewportBorders(currentVP);

        return true;
}

uint32_t Renderer::retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width) {
    int32_t i;
    /* 8192 is really arbitrary. It should be a value dependent on the
    number of nodes / segments */
    GLuint selectionBuffer[8192] = {0};
    GLint hits, openGLviewport[4];
    GLuint names, *ptr, minZ, *ptrName;
    ptrName = NULL;

    glViewport(state->viewerState->vpConfigs[currentVP].upperLeftCorner.x,
        state->viewerState->screenSizeY
        - state->viewerState->vpConfigs[currentVP].upperLeftCorner.y
        - state->viewerState->vpConfigs[currentVP].edgeLength,
        state->viewerState->vpConfigs[currentVP].edgeLength,
        state->viewerState->vpConfigs[currentVP].edgeLength);

    glGetIntegerv(GL_VIEWPORT, openGLviewport);

    glSelectBuffer(8192, selectionBuffer);

    state->viewerState->selectModeFlag = true;

    glRenderMode(GL_SELECT);

    glInitNames();
    glPushName(0);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    gluPickMatrix(x, y, (float)width, (float)width, openGLviewport);

    if(state->viewerState->vpConfigs[currentVP].type == VIEWPORT_SKELETON) {
        glOrtho(state->skeletonState->volBoundary
            * state->skeletonState->zoomLevel
            + state->skeletonState->translateX,
            state->skeletonState->volBoundary
            - (state->skeletonState->volBoundary
            * state->skeletonState->zoomLevel)
            + state->skeletonState->translateX,
            state->skeletonState->volBoundary
            - (state->skeletonState->volBoundary
            * state->skeletonState->zoomLevel)
            + state->skeletonState->translateY,
              state->skeletonState->volBoundary
            * state->skeletonState->zoomLevel
            + state->skeletonState->translateY,
            -10000, 10 * state->skeletonState->volBoundary);
        glCallList(state->skeletonState->displayListView);
        glCallList(state->skeletonState->displayListSkeletonSkeletonizerVP);
        glCallList(state->skeletonState->displayListDataset); //TDitem fix that display list !!

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_MULTISAMPLE);
    }
    else {
        //glEnable(GL_DEPTH_TEST);
        //glCallList(state->viewerState->vpConfigs[currentVP].displayList);
        glDisable(GL_DEPTH_TEST);
        renderOrthogonalVP(currentVP);
    }

    hits = glRenderMode(GL_RENDER);
    glLoadIdentity();

    ptr = (GLuint *)selectionBuffer;

    minZ = 0xffffffff;


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
    }
    else {
        return false;
    }
}

//Some math helper functions
float Renderer::radToDeg(float rad) {
    return ((180. * rad) / PI);
}

float Renderer::degToRad(float deg) {
    return ((deg / 180.) * PI);
}

float Renderer::scalarProduct(floatCoordinate *v1, floatCoordinate *v2) {
    return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));
}

floatCoordinate* Renderer::crossProduct(floatCoordinate *v1, floatCoordinate *v2) {
    floatCoordinate *result = NULL;
    result = (floatCoordinate*)malloc(sizeof(floatCoordinate));
    result->x = v1->y * v2->z - v1->z * v2->y;
    result->y = v1->z * v2->x - v1->x * v2->z;
    result->z = v1->x * v2->y - v1->y * v2->x;
    return result;
}

float Renderer::vectorAngle(floatCoordinate *v1, floatCoordinate *v2) {
    return ((float)acos((double)(scalarProduct(v1, v2)) / (euclidicNorm(v1)*euclidicNorm(v2))));
}

float Renderer::euclidicNorm(floatCoordinate *v) {
    return ((float)sqrt((double)scalarProduct(v, v)));
}

bool Renderer::normalizeVector(floatCoordinate *v) {
    float norm = euclidicNorm(v);
    v->x /= norm;
    v->y /= norm;
    v->z /= norm;
    return true;
}

int32_t Renderer::roundFloat(float number) {
    if(number >= 0) return (int32_t)(number + 0.5);
    else return (int32_t)(number - 0.5);
}

int32_t Renderer::sgn(float number) {
    if(number > 0.) return 1;
    else if(number == 0.) return 0;
    else return -1;
}

bool Renderer::initRenderer() {
    /* initialize the textures used to display the SBFSEM data TDitem: return val check*/
    Viewer::initializeTextures();
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
    //LOG("state->viewerState->voxelXYtoZRatio = %f", state->viewerState->voxelXYtoZRatio);
    glRotatef(235., 1., 0., 0.);
    glRotatef(210., 0., 0., 1.);
    setRotationState(ROTATIONSTATERESET);
    //glScalef(1., 1., 1./state->viewerState->voxelXYtoZRatio);
    /* save the matrix for further use... */
    glGetFloatv(GL_MODELVIEW_MATRIX, state->skeletonState->skeletonVpModelView);

    glLoadIdentity();

    return true;
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


bool Renderer::setRotationState(uint32_t setTo) {
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


static GLuint renderActiveTreeSkeleton(Byte callFlag) {
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode;
    struct segmentListElement *currentSegment;

    char *textBuffer;
    textBuffer = (char*)malloc(32);
    memset(textBuffer, '\0', 32);

    GLuint tempList;
    tempList = glGenLists(1);
    glNewList(tempList, GL_COMPILE);
    if(callFlag) glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Save current matrix stack (modelview!!)
    glPushMatrix();

    //Rendering of objects has to start always at the origin of our data pixel coordinate system.
    //Thus, we have to translate there.
    glTranslatef(-(float)state->boundary.x / 2. + 0.5,-(float)state->boundary.y / 2. + 0.5,-(float)state->boundary.z / 2. + 0.5);


    if(state->skeletonState->activeTree) {
        currentTree = state->skeletonState->activeTree;

        currentNode = currentTree->firstNode;
        while(currentNode) {
            //Set color
            if(currentNode->isBranchNode) {
               glColor4f(0., 0., 1., 1.);
            }
            if(currentNode->comment != NULL) {
                glColor4f(1., 1., 0., 1.);
            }
            else {
                if(state->skeletonState->highlightActiveTree) {
                    glColor4f(1., 0., 0., 1.);
                }
                else {
                    glColor4f(currentTree->color.r, currentTree->color.g, currentTree->color.b, currentTree->color.a);
                }
            }

            //The first 50 entries of the openGL namespace are reserved for static objects (like slice plane quads...)
            glLoadName(currentNode->nodeID + 50);
            if(state->skeletonState->overrideNodeRadiusBool) {
                renderSphere(&(currentNode->position),
                             state->skeletonState->overrideNodeRadiusVal);
            }
            else {
                renderSphere(&(currentNode->position),
                             currentNode->radius);
            }

            if(state->skeletonState->highlightActiveTree) glColor4f(1., 0., 0., 1.);
            else glColor4f(currentTree->color.r, currentTree->color.g, currentTree->color.b, currentTree->color.a);

            currentSegment = currentNode->firstSegment;
            while(currentSegment) {
                //2 indicates a backward connection, which should not be rendered.
                if(currentSegment->flag == 2) {
                    currentSegment = currentSegment->next;
                    continue;
                }
                glLoadName(3);
                if(state->skeletonState->overrideNodeRadiusBool)
                    renderCylinder(&(currentSegment->source->position),
                                   state->skeletonState->overrideNodeRadiusVal
                                   * state->skeletonState->segRadiusToNodeRadius,
                                   &(currentSegment->target->position),
                                   state->skeletonState->overrideNodeRadiusVal
                                   * state->skeletonState->segRadiusToNodeRadius);
                else
                    renderCylinder(&(currentSegment->source->position),
                                   currentSegment->source->radius
                                   * state->skeletonState->segRadiusToNodeRadius,
                                   &(currentSegment->target->position),
                                   currentSegment->target->radius
                                   * state->skeletonState->segRadiusToNodeRadius);
                    //Gets true, if called for slice plane VP
                    if(!callFlag) {
                        if(state->skeletonState->showIntersections)
                            renderSegPlaneIntersection(currentSegment);
                    }

                currentSegment = currentSegment->next;
            }
            //Render the node description only when option is set.
            if(state->skeletonState->showNodeIDs) {
                glColor4f(0., 0., 0., 1.);
                memset(textBuffer, '\0', 32);
                sprintf(textBuffer, "%d", currentNode->nodeID);
                renderText(&(currentNode->position),
                    textBuffer);
            }
            currentNode = currentNode->next;
        }
        //Highlight active node
        if(state->skeletonState->activeNode) {
            if(state->skeletonState->activeNode->correspondingTree == currentTree) {
                if(state->skeletonState->activeNode->isBranchNode) {
                    glColor4f(0., 0., 1., 0.2);
                }
                else if(state->skeletonState->activeNode->comment != NULL) {
                    glColor4f(1., 1., 0., 0.2);
                }
                else {
                    glColor4f(1.0, 0., 0., 0.2);
                }
                glEnable(GL_BLEND);
                glLoadName(state->skeletonState->activeNode->nodeID + 50);
                if(state->skeletonState->overrideNodeRadiusBool)
                    renderSphere(&(state->skeletonState->activeNode->position),
                                 state->skeletonState->overrideNodeRadiusVal * 1.5);
                else
                    renderSphere(&(state->skeletonState->activeNode->position),
                                 state->skeletonState->activeNode->radius * 1.5);
                glDisable(GL_BLEND);
                //Description of active node is always rendered, ignoring state->skeletonState->showNodeIDs
                glColor4f(0., 0., 0., 1.);
                memset(textBuffer, '\0', 32);
                sprintf(textBuffer, "%d", state->skeletonState->activeNode->nodeID);
                renderText(&(state->skeletonState->activeNode->position),
                           textBuffer);
            }
        }
    }
    //Restore modelview matrix
    glPopMatrix();
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Stop display list recording
    glEndList();
    free(textBuffer);
    return tempList;
}

static GLuint renderSuperCubeSkeleton(Byte callFlag) {
    Coordinate currentPosDC, currentPosDCCounter;

    struct skeletonDC *currentSkeletonDC;
    struct skeletonDCnode *currentSkeletonDCnode;
    struct skeletonDCsegment *currentSkeletonDCsegment;
    struct skeletonDCsegment *firstRenderedSkeletonDCsegment;
    struct skeletonDCsegment *currentSkeletonDCsegmentSearch;

    Byte rendered = false;

    firstRenderedSkeletonDCsegment = (skeletonDCsegment*)malloc(sizeof(struct skeletonDCsegment));
    memset(firstRenderedSkeletonDCsegment, '\0', sizeof(struct skeletonDCsegment));

    currentPosDC = Coordinate::Px2DcCoord(state->viewerState->currentPosition);

    char *textBuffer;
    textBuffer = (char*)malloc(32);
    memset(textBuffer, '\0', 32);

    GLuint tempList;
    tempList = glGenLists(1);
    glNewList(tempList, GL_COMPILE);

    if(callFlag) glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Save current matrix stack (modelview!!)
    glPushMatrix();

    //Rendering of objects has to start always at the origin of our data pixel coordinate system.
    //Thus, we have to translate there.
    glTranslatef(-(float)state->boundary.x / 2. + 0.5,-(float)state->boundary.y / 2. + 0.5,-(float)state->boundary.z / 2. + 0.5);


    //We take all skeletonDCs out of our current SC
    for(currentPosDCCounter.x = currentPosDC.x - state->viewerState->zoomCube; currentPosDCCounter.x <= currentPosDC.x + state->viewerState->zoomCube; currentPosDCCounter.x++) {
        for(currentPosDCCounter.y = currentPosDC.y - state->viewerState->zoomCube; currentPosDCCounter.y <= currentPosDC.y + state->viewerState->zoomCube; currentPosDCCounter.y++) {
            for(currentPosDCCounter.z = currentPosDC.z - state->viewerState->zoomCube; currentPosDCCounter.z <= currentPosDC.z + state->viewerState->zoomCube; currentPosDCCounter.z++) {
                currentSkeletonDC = (struct skeletonDC *)Hashtable::ht_get(state->skeletonState->skeletonDCs, currentPosDCCounter);

                //If there is a valid skeletonDC, there are nodes / segments (or both) in it.
                if(currentSkeletonDC != HT_FAILURE) {

                    //Go through all segments of the data cube
                    if(currentSkeletonDC->firstSkeletonDCsegment) {
                        currentSkeletonDCsegment = currentSkeletonDC->firstSkeletonDCsegment;

                        while(currentSkeletonDCsegment) {

                            //Check if this segment has already been rendered and skip it if true.
                            //No segment rendered yet.. this is the first one
                            if(firstRenderedSkeletonDCsegment->segment == NULL) {
                                //Set color
                                if((currentSkeletonDCsegment->segment->source->correspondingTree == state->skeletonState->activeTree)
                                    && (state->skeletonState->highlightActiveTree)) {
                                        glColor4f(1., 0., 0., 1.);
                                }
                                else
                                    glColor4f(currentSkeletonDCsegment->segment->source->correspondingTree->color.r,
                                              currentSkeletonDCsegment->segment->source->correspondingTree->color.g,
                                              currentSkeletonDCsegment->segment->source->correspondingTree->color.b,
                                              currentSkeletonDCsegment->segment->source->correspondingTree->color.a);
                                glLoadName(3);
                                if(state->skeletonState->overrideNodeRadiusBool)
                                    renderCylinder(&(currentSkeletonDCsegment->segment->source->position),
                                                   state->skeletonState->overrideNodeRadiusVal *
                                                    state->skeletonState->segRadiusToNodeRadius,
                                                   &currentSkeletonDCsegment->segment->target->position,
                                                   state->skeletonState->overrideNodeRadiusVal *
                                                    state->skeletonState->segRadiusToNodeRadius);
                                else
                                    renderCylinder(&(currentSkeletonDCsegment->segment->source->position),
                                        currentSkeletonDCsegment->segment->source->radius * state->skeletonState->segRadiusToNodeRadius,
                                        &(currentSkeletonDCsegment->segment->target->position),
                                        currentSkeletonDCsegment->segment->target->radius * state->skeletonState->segRadiusToNodeRadius);

                                //Gets true, if called for slice plane VP
                                if(!callFlag) {
                                    if(state->skeletonState->showIntersections)
                                        renderSegPlaneIntersection(currentSkeletonDCsegment->segment);
                                }

                                //A bit hackish to use the same struct for it, I know...
                                firstRenderedSkeletonDCsegment->segment = (struct segmentListElement *)currentSkeletonDCsegment;
                            }
                            else {
                                rendered = false;
                                currentSkeletonDCsegmentSearch = firstRenderedSkeletonDCsegment;
                                //Check all segments in the current SC if they were rendered...
                                /* this adds O(n^2) :( ) */
                                /*while(currentSkeletonDCsegmentSearch) {
                                    //Already rendered, skip this
                                    if(((struct skeletonDCsegment *)currentSkeletonDCsegmentSearch->segment) == currentSkeletonDCsegment) {
                                        //currentSkeletonDCsegmentSearch = currentSkeletonDCsegmentSearch->next;
                                        rendered = true;
                                        break;
                                    }
                                    currentSkeletonDCsegmentSearch = currentSkeletonDCsegmentSearch->next;
                                }*/
                                //So render it and add a new element to the list of rendered segments
                                if(rendered == false) {
                                    //Set color
                                    if((currentSkeletonDCsegment->segment->source->correspondingTree == state->skeletonState->activeTree)
                                        && (state->skeletonState->highlightActiveTree))
                                        glColor4f(1., 0., 0., 1.);
                                    else
                                        glColor4f(currentSkeletonDCsegment->segment->source->correspondingTree->color.r,
                                                  currentSkeletonDCsegment->segment->source->correspondingTree->color.g,
                                                  currentSkeletonDCsegment->segment->source->correspondingTree->color.b,
                                                  currentSkeletonDCsegment->segment->source->correspondingTree->color.a);
                                    glLoadName(3);
                                    if(state->skeletonState->overrideNodeRadiusBool)
                                        renderCylinder(&(currentSkeletonDCsegment->segment->source->position),
                                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius,
                                            &(currentSkeletonDCsegment->segment->target->position),
                                            state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius);
                                    else
                                        renderCylinder(&(currentSkeletonDCsegment->segment->source->position),
                                            currentSkeletonDCsegment->segment->source->radius * state->skeletonState->segRadiusToNodeRadius,
                                            &(currentSkeletonDCsegment->segment->target->position),
                                            currentSkeletonDCsegment->segment->target->radius * state->skeletonState->segRadiusToNodeRadius);

                                    //Gets true, if called for slice plane VP
                                    if(!callFlag) {
                                        if(state->skeletonState->showIntersections)
                                            renderSegPlaneIntersection(currentSkeletonDCsegment->segment);
                                    }

                                    currentSkeletonDCsegmentSearch = (skeletonDCsegment*)malloc(sizeof(struct skeletonDCsegment));
                                    memset(currentSkeletonDCsegmentSearch, '\0', sizeof(struct skeletonDCsegment));

                                    currentSkeletonDCsegmentSearch->next = firstRenderedSkeletonDCsegment;
                                    firstRenderedSkeletonDCsegment = currentSkeletonDCsegmentSearch;
                                }
                            }

                            currentSkeletonDCsegment = currentSkeletonDCsegment->next;
                        }
                    }

                    //Go through all nodes of the SC
                    if(currentSkeletonDC->firstSkeletonDCnode) {
                        currentSkeletonDCnode = currentSkeletonDC->firstSkeletonDCnode;

                        while(currentSkeletonDCnode) {
                            //Set color
                            if((currentSkeletonDCnode->node->correspondingTree == state->skeletonState->activeTree)
                                && (state->skeletonState->highlightActiveTree)) {
                                    glColor4f(1., 0., 0., 1.);
                            }
                            else
                                glColor4f(currentSkeletonDCnode->node->correspondingTree->color.r,
                                          currentSkeletonDCnode->node->correspondingTree->color.g,
                                          currentSkeletonDCnode->node->correspondingTree->color.b,
                                          currentSkeletonDCnode->node->correspondingTree->color.a);

                            if(currentSkeletonDCnode->node->isBranchNode) {
                                glColor4f(0., 0., 1., 1.);
                            }
                            else if(currentSkeletonDCnode->node->comment != NULL) {
                                glColor4f(1., 1., 0., 1.);
                            }
                            //The first 50 entries of the openGL namespace are reserved for static objects (like slice plane quads...)
                            glLoadName(currentSkeletonDCnode->node->nodeID + 50);
                            //renderSphere(&(currentSkeletonDCnode->node->position), currentSkeletonDCnode->node->radius);
                            if(state->skeletonState->overrideNodeRadiusBool)
                                renderSphere(&(currentSkeletonDCnode->node->position), state->skeletonState->overrideNodeRadiusVal);
                            else
                                renderSphere(&(currentSkeletonDCnode->node->position), currentSkeletonDCnode->node->radius);

                            //Check if this node is an active node and highlight if true
                            if(state->skeletonState->activeNode) {
                                if(currentSkeletonDCnode->node->nodeID == state->skeletonState->activeNode->nodeID) {
                                    if(currentSkeletonDCnode->node->isBranchNode) {
                                        glColor4f(0., 0., 1.0, 0.2);
                                    }
                                    else if(currentSkeletonDCnode->node->comment != NULL) {
                                        glColor4f(1., 1., 0., 0.2);
                                    }
                                    else {
                                        glColor4f(1., 0., 0., 0.2);
                                    }
                                    glLoadName(currentSkeletonDCnode->node->nodeID + 50);
                                    glEnable(GL_BLEND);
                                    //renderSphere(&(currentSkeletonDCnode->node->position), currentSkeletonDCnode->node->radius * 1.5);
                                    if(state->skeletonState->overrideNodeRadiusBool)
                                        renderSphere(&(currentSkeletonDCnode->node->position), state->skeletonState->overrideNodeRadiusVal);
                                    else
                                        renderSphere(&(currentSkeletonDCnode->node->position), currentSkeletonDCnode->node->radius * 1.5);
                                    glDisable(GL_BLEND);
                                    //Description of active node is always rendered, ignoring state->skeletonState->showNodeIDs
                                    glColor4f(0., 0., 0., 1.);
                                    memset(textBuffer, '\0', 32);
                                    sprintf(textBuffer, "%d", state->skeletonState->activeNode->nodeID);
                                    renderText(&(currentSkeletonDCnode->node->position), textBuffer);
                                }
                            }

                            //Render the node description only when option is set.
                            if(state->skeletonState->showNodeIDs) {
                                glColor4f(0., 0., 0., 1.);
                                memset(textBuffer, '\0', 32);
                                sprintf(textBuffer, "%d", currentSkeletonDCnode->node->nodeID);
                                renderText(&(currentSkeletonDCnode->node->position), textBuffer);
                            }
                            currentSkeletonDCnode = currentSkeletonDCnode->next;
                        }
                    }
                }
            }
        }
    }

    //Now we have to clean up our list of rendered segments...
    while(firstRenderedSkeletonDCsegment) {
        currentSkeletonDCsegmentSearch = firstRenderedSkeletonDCsegment->next;
        free(firstRenderedSkeletonDCsegment);
        firstRenderedSkeletonDCsegment = currentSkeletonDCsegmentSearch;
    }

    //Restore modelview matrix
    glPopMatrix();
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Stop display list recording
    glEndList();

    free(textBuffer);

    return tempList;
}

static GLuint renderWholeSkeleton(Byte callFlag) {
    struct treeListElement *currentTree;
    struct nodeListElement *currentNode;
    struct segmentListElement *currentSegment;

    char *textBuffer;
    textBuffer = (char*)malloc(32);
    memset(textBuffer, '\0', 32);

    GLuint tempList;
    tempList = glGenLists(1);
    glNewList(tempList, GL_COMPILE);
    if(callFlag) glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Save current matrix stack (modelview!!)
    glPushMatrix();

    //Rendering of objects has to start always at the origin of our data pixel coordinate system.
    //Thus, we have to translate there.
    glTranslatef(-(float)state->boundary.x / 2. + 0.5,-(float)state->boundary.y / 2. + 0.5,-(float)state->boundary.z / 2. + 0.5);

    //We iterate over the whole tree structure here.
    currentTree = state->skeletonState->firstTree;

    while(currentTree) {
        currentNode = currentTree->firstNode;
        while(currentNode) {
            //Set color
            if((currentTree->treeID == state->skeletonState->activeTree->treeID)
                && (state->skeletonState->highlightActiveTree)) {
                    glColor4f(1., 0., 0., 1.);
            }
            else
                glColor4f(currentTree->color.r,
                          currentTree->color.g,
                          currentTree->color.b,
                          currentTree->color.a);

            if(currentNode->isBranchNode) {
                glColor4f(0., 0., 1., 1.);
            }
            else if(currentNode->comment != NULL) {
                glColor4f(1., 1., 0., 1.);
            }

            //The first 50 entries of the openGL namespace are reserved for static objects (like slice plane quads...)
            glLoadName(currentNode->nodeID + 50);
            if(state->skeletonState->overrideNodeRadiusBool)
                renderSphere(&(currentNode->position), state->skeletonState->overrideNodeRadiusVal);
            else
                renderSphere(&(currentNode->position), currentNode->radius);

            if((currentTree->treeID == state->skeletonState->activeTree->treeID)
                && (state->skeletonState->highlightActiveTree)) {
                    glColor4f(1., 0., 0., 1.);
            }
            else
                glColor4f(currentTree->color.r,
                          currentTree->color.g,
                          currentTree->color.b,
                          currentTree->color.a);
            currentSegment = currentNode->firstSegment;
            while(currentSegment) {
                //2 indicates a backward connection, which should not be rendered.
                if(currentSegment->flag == 2) {
                    currentSegment = currentSegment->next;
                    continue;
                }
                glLoadName(3);
                if(state->skeletonState->overrideNodeRadiusBool)
                    renderCylinder(&(currentSegment->source->position),
                        state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius,
                        &(currentSegment->target->position),
                        state->skeletonState->overrideNodeRadiusVal * state->skeletonState->segRadiusToNodeRadius);
                else
                    renderCylinder(&(currentSegment->source->position),
                        currentSegment->source->radius * state->skeletonState->segRadiusToNodeRadius,
                        &(currentSegment->target->position),
                        currentSegment->target->radius * state->skeletonState->segRadiusToNodeRadius);

                //Gets true, if called for slice plane VP
                if(!callFlag) {
                    if(state->skeletonState->showIntersections)
                        renderSegPlaneIntersection(currentSegment);
                }

                currentSegment = currentSegment->next;

            }
            //Render the node description only when option is set.
            if(state->skeletonState->showNodeIDs) {
                glColor4f(0., 0., 0., 1.);
                memset(textBuffer, '\0', 32);
                sprintf(textBuffer, "%d", currentNode->nodeID);
                renderText(&(currentNode->position), textBuffer);
            }

            currentNode = currentNode->next;
        }

        currentTree = currentTree->next;
    }

    //Highlight active node
    if(state->skeletonState->activeNode) {
        if(state->skeletonState->activeNode->isBranchNode) {
            glColor4f(0., 0., 1., 0.2);
        }
        else if(state->skeletonState->activeNode->comment != NULL) {
            glColor4f(1., 1., 0., 0.2);
        }
        else {
            glColor4f(1.0, 0., 0., 0.2);
        }

        glLoadName(state->skeletonState->activeNode->nodeID + 50);
        glEnable(GL_BLEND);
        if(state->skeletonState->overrideNodeRadiusBool)
            renderSphere(&(state->skeletonState->activeNode->position), state->skeletonState->overrideNodeRadiusVal * 1.5);
        else
            renderSphere(&(state->skeletonState->activeNode->position), state->skeletonState->activeNode->radius * 1.5);
        glDisable(GL_BLEND);
        //Description of active node is always rendered, ignoring state->skeletonState->showNodeIDs
        glColor4f(0., 0., 0., 1.);
        memset(textBuffer, '\0', 32);
        sprintf(textBuffer, "%d", state->skeletonState->activeNode->nodeID);
        renderText(&(state->skeletonState->activeNode->position), textBuffer);
        /*if(state->skeletonState->activeNode->comment) {
            glPushMatrix();
            glTranslated(10,10, 10);
            renderText(&(state->skeletonState->activeNode->position), state->skeletonState->activeNode->comment);
            glPopMatrix();
        }*/

    }
    //Restore modelview matrix
    glPopMatrix();
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    //Stop display list recording
    glEndList();

    free(textBuffer);

    return tempList;
}


static uint32_t updateDisplayListsSkeleton() {

    if(state->skeletonState->skeletonChanged) {
        state->skeletonState->skeletonChanged = false;
        state->viewerState->superCubeChanged = false;
        state->skeletonState->skeletonSliceVPchanged = false;

        /* clean up the old display lists */
        if(state->skeletonState->displayListSkeletonSkeletonizerVP) {
            glDeleteLists(state->skeletonState->displayListSkeletonSkeletonizerVP, 1);
            state->skeletonState->displayListSkeletonSkeletonizerVP = 0;
        }
        if(state->skeletonState->displayListSkeletonSlicePlaneVP) {
            glDeleteLists(state->skeletonState->displayListSkeletonSlicePlaneVP, 1);
            state->skeletonState->displayListSkeletonSlicePlaneVP = 0;
        }

        /* create new display lists that are up-to-date */
        if(state->skeletonState->displayMode & DSP_SKEL_VP_WHOLE) {
            state->skeletonState->displayListSkeletonSkeletonizerVP =
                renderWholeSkeleton(false);
            if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE))
                state->skeletonState->displayListSkeletonSlicePlaneVP =
                    //renderSuperCubeSkeleton(0);
                    renderWholeSkeleton(false);

        }

        if(state->skeletonState->displayMode & DSP_SKEL_VP_HIDE) {
            if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE))
                state->skeletonState->displayListSkeletonSlicePlaneVP =
                    //renderSuperCubeSkeleton(false);
                    renderWholeSkeleton(false);

        }

        if(state->skeletonState->displayMode & DSP_SKEL_VP_CURRENTCUBE) {
            state->skeletonState->displayListSkeletonSkeletonizerVP =
                renderSuperCubeSkeleton(true);
            if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE)) {
                if(state->skeletonState->showIntersections)
                    state->skeletonState->displayListSkeletonSlicePlaneVP =
                        //renderSuperCubeSkeleton(false);
                        renderWholeSkeleton(false);

                else state->skeletonState->displayListSkeletonSlicePlaneVP =
                    state->skeletonState->displayListSkeletonSkeletonizerVP;
            }
        }
        /* TDitem active tree should be limited to current cube in this case */
        if(state->skeletonState->displayMode & DSP_ACTIVETREE) {

            state->skeletonState->displayListSkeletonSkeletonizerVP =
                renderActiveTreeSkeleton(1);
            if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE)) {
                if(state->skeletonState->showIntersections)
                    state->skeletonState->displayListSkeletonSlicePlaneVP =
                        renderActiveTreeSkeleton(0);
                else state->skeletonState->displayListSkeletonSlicePlaneVP =
                    state->skeletonState->displayListSkeletonSkeletonizerVP;
            }
        }
    }

    if(state->viewerState->superCubeChanged) {
        state->viewerState->superCubeChanged = false;

        if(state->skeletonState->displayMode & DSP_SKEL_VP_CURRENTCUBE) {
            glDeleteLists(state->skeletonState->displayListSkeletonSkeletonizerVP, 1);
            state->skeletonState->displayListSkeletonSkeletonizerVP = 0;

            state->skeletonState->displayListSkeletonSkeletonizerVP =
                renderSuperCubeSkeleton(1);
        }

        if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE)) {
            if(!(state->skeletonState->displayMode & DSP_ACTIVETREE)) {
                state->skeletonState->displayListSkeletonSlicePlaneVP =
                    //renderSuperCubeSkeleton(0);
                    renderWholeSkeleton(false);

            }
        }
    }

    if(state->skeletonState->skeletonSliceVPchanged) {
        state->skeletonState->skeletonSliceVPchanged = false;
        glDeleteLists(state->skeletonState->displayListSkeletonSlicePlaneVP, 1);
        state->skeletonState->displayListSkeletonSlicePlaneVP = 0;

        if(!(state->skeletonState->displayMode & DSP_SLICE_VP_HIDE)) {
            if(state->skeletonState->displayMode & DSP_ACTIVETREE) {
                state->skeletonState->displayListSkeletonSlicePlaneVP =
                    renderActiveTreeSkeleton(0);
            }
            else {
                state->skeletonState->displayListSkeletonSlicePlaneVP =
                    renderWholeSkeleton(0);
                    //renderSuperCubeSkeleton(0);
            }
        }
    }

    return true;
}


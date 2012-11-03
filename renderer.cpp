#include "renderer.h"

//static functions in Knossos 3.x
static GLuint renderWholeSkeleton(Byte callFlag) { return 0;}
static GLuint renderSuperCubeSkeleton(Byte callFlag) { return 0;}
static GLuint renderActiveTreeSkeleton(Byte callFlag) { return 0;}
static uint32_t renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius) { return 0;}
static uint32_t renderSphere(Coordinate *pos, float radius) { return 0;}
static uint32_t renderText(Coordinate *pos, char *string) { return 0;}
static uint32_t updateDisplayListsSkeleton() { return 0;}
static uint32_t renderSegPlaneIntersection(struct segmentListElement *segment) { return 0;}
static uint32_t renderViewportBorders(uint32_t currentVP) { return 0;}

Renderer::Renderer(QObject *parent) :
    QObject(parent)
{
}

bool Renderer::drawGUI() {
 return true;
}

bool Renderer::renderOrthogonalVP(uint32_t currentVP) {
 return true;
}

bool Renderer::renderSkeletonVP(uint32_t currentVP) {
 return true;
}

uint32_t Renderer::retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width) {
 return 0;
}

//Some math helper functions
float Renderer::radToDeg(float rad) {
 return 0;
}

float Renderer::degToRad(float deg) {
 return 0;
}

float Renderer::scalarProduct(floatCoordinate *v1, floatCoordinate *v2) {
 return 0;
}

floatCoordinate* Renderer::crossProduct(floatCoordinate *v1, floatCoordinate *v2) {
 return NULL;
}

float Renderer::vectorAngle(floatCoordinate *v1, floatCoordinate *v2) {
 return 0;
}

float Renderer::euclidicNorm(floatCoordinate *v) {
 return 0;
}

bool Renderer::normalizeVector(floatCoordinate *v) {
 return true;
}

int32_t Renderer::roundFloat(float number) {
 return 0;
}

int32_t Renderer::sgn(float number) {
 return 0;
}

bool Renderer::initRenderer() {
 return true;
}

bool Renderer::splashScreen() {
 return true;
}

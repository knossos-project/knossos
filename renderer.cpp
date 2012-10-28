#include "renderer.h"

//static functions in Knossos 3.x
static GLuint renderWholeSkeleton(Byte callFlag) {}
static GLuint renderSuperCubeSkeleton(Byte callFlag) {}
static GLuint renderActiveTreeSkeleton(Byte callFlag) {}
static uint32_t renderCylinder(Coordinate *base, float baseRadius, Coordinate *top, float topRadius) {}
static uint32_t renderSphere(Coordinate *pos, float radius) {}
static uint32_t renderText(Coordinate *pos, char *string) {}
static uint32_t updateDisplayListsSkeleton() {}
static uint32_t renderSegPlaneIntersection(struct segmentListElement *segment) {}
static uint32_t renderViewportBorders(uint32_t currentVP) {}

Renderer::Renderer(QObject *parent) :
    QObject(parent)
{
}

bool Renderer::drawGUI() {

}

bool Renderer::renderOrthogonalVP(uint32_t currentVP) {

}

bool Renderer::renderSkeletonVP(uint32_t currentVP) {

}

uint32_t Renderer::retrieveVisibleObjectBeneathSquare(uint32_t currentVP, uint32_t x, uint32_t y, uint32_t width) {

}

//Some math helper functions
float Renderer::radToDeg(float rad) {

}

float Renderer::degToRad(float deg) {

}

float Renderer::scalarProduct(floatCoordinate *v1, floatCoordinate *v2) {

}

floatCoordinate* Renderer::crossProduct(floatCoordinate *v1, floatCoordinate *v2) {

}

float Renderer::vectorAngle(floatCoordinate *v1, floatCoordinate *v2) {

}

float Renderer::euclidicNorm(floatCoordinate *v) {

}

bool Renderer::normalizeVector(floatCoordinate *v) {

}

int32_t Renderer::roundFloat(float number) {

}

int32_t Renderer::sgn(float number) {

}

bool Renderer::initRenderer() {

}

bool Renderer::splashScreen() {

}

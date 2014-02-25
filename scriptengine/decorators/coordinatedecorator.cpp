#include "coordinatedecorator.h"
#include "knossos-global.h"

CoordinateDecorator::CoordinateDecorator(QObject *parent) :
    QObject(parent)
{
}

Coordinate *CoordinateDecorator::new_Coordinate() {
    return new Coordinate();
}

Coordinate *CoordinateDecorator::new_Coordinate(int x, int y, int z) {
    return new Coordinate(x, y, z);
}

int CoordinateDecorator::getX(Coordinate *self) {
    return self->x;
}

int CoordinateDecorator::getY(Coordinate *self) {
    return self->y;
}

int CoordinateDecorator::getZ(Coordinate *self) {
    return self->z;
}

void CoordinateDecorator::setX(Coordinate *self, int x) {
    self->setX(x);
}

void CoordinateDecorator::setY(Coordinate *self, int y) {
    self->setY(y);
}

void CoordinateDecorator::setZ(Coordinate *self, int z) {
    self->setZ(z);
}


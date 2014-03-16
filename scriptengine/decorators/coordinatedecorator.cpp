#include "coordinatedecorator.h"
#include "knossos-global.h"

CoordinateDecorator::CoordinateDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
Coordinate *CoordinateDecorator::new_Coordinate() {
    return new Coordinate();
}

Coordinate *CoordinateDecorator::new_Coordinate(int x, int y, int z) {
    return new Coordinate(x, y, z);
}
*/

int CoordinateDecorator::x(Coordinate *self) {
    return self->x;
}

int CoordinateDecorator::y(Coordinate *self) {
    return self->y;
}

int CoordinateDecorator::z(Coordinate *self) {
    return self->z;
}

/*
void CoordinateDecorator::setx(Coordinate *self, int x) {
    self->x = x;
}

void CoordinateDecorator::sety(Coordinate *self, int y) {
    self->y = y;
}

void CoordinateDecorator::setz(Coordinate *self, int z) {
    self->z = z;
}
*/

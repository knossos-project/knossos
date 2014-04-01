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


int CoordinateDecorator::x(Coordinate *self) {
    return self->x;
}

int CoordinateDecorator::y(Coordinate *self) {
    return self->y;
}

int CoordinateDecorator::z(Coordinate *self) {
    return self->z;
}

QString CoordinateDecorator::static_Coordinate_help() {
    return QString("An instanceable class which stores 3D coordinate in integer values. Access to attributes only via getter and setter." \
                   "\n\n Constructors: " \
                   "\n Coordinate() : creates an empty coordinate object." \
                   "\n Coordinate(x, y, z) : creates a coordinate objects where each dimension is specified." \
                   "\n\n GETTER: " \
                   "\n x() : returns the x value of the coordinate" \
                   "\n y() : returns the y value of the coordinate" \
                   "\n z() : returns the z value of the coordinate" \
                   "\n\n SETTER: " \
                   "\n setx(x) : sets the x value of the coordinate, expects integer " \
                   "\n sety(y) : sets the y value of the coordinate, expects integer " \
                   "\n setz(z) : sets the z value of the coordinate, excpets integer "
                   );
}


void CoordinateDecorator::setx(Coordinate *self, int x) {
    self->x = x;
}

void CoordinateDecorator::sety(Coordinate *self, int y) {
    self->y = y;
}

void CoordinateDecorator::setz(Coordinate *self, int z) {
    self->z = z;
}


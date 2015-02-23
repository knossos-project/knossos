#include "coordinatedecorator.h"

CoordinateDecorator::CoordinateDecorator(QObject *parent) : QObject(parent) {
    qRegisterMetaType<Coordinate>();
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
    return QString("An instanceable class which stores 3D Coordinate in integer values. Access to attributes only via getter and setter." \
                   "\n\n Constructors: " \
                   "\n Coordinate() : creates an empty Coordinate object." \
                   "\n Coordinate(x, y, z) : creates a Coordinate objects where each dimension is specified." \
                   "\n\n GETTER: " \
                   "\n x() : returns the x value of the Coordinate" \
                   "\n y() : returns the y value of the Coordinate" \
                   "\n z() : returns the z value of the Coordinate" \
                   "\n\n SETTER: " \
                   "\n setx(x) : sets the x value of the Coordinate, expects integer " \
                   "\n sety(y) : sets the y value of the Coordinate, expects integer " \
                   "\n setz(z) : sets the z value of the Coordinate, excpets integer "
                   );
}

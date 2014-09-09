#include "floatcoordinatedecorator.h"
#include "knossos-global.h"
#include <QDebug>




FloatCoordinateDecorator::FloatCoordinateDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<floatCoordinate>();
}

floatCoordinate *FloatCoordinateDecorator::new_floatCoordinate() {
    return new floatCoordinate();
}

floatCoordinate *FloatCoordinateDecorator::new_floatCoordinate(float x, float y, float z) {
    return new floatCoordinate{x, y, z};
}

float FloatCoordinateDecorator::x(floatCoordinate *self) {
    return self->x;
}

float FloatCoordinateDecorator::y(floatCoordinate *self) {
    return self->y;
}

float FloatCoordinateDecorator::z(floatCoordinate *self) {
    return self->z;
}

QString FloatCoordinateDecorator::static_floatCoordinate_help() {
    return QString("An instanceable class storing 3D coordinates as float. Access to attributes only via getter and setter." \
                   "\n\n CONSTRUCTORS: " \
                   "\n FCoordinate() : creates an empty floatCoordinate object. " \
                   "\n FCoordinate(x, y, z) : creates a FloatCoordinates object where all dimensions are to specify." \
                   "\n\n GETTER: " \
                   "\n x() : returns the x value of the Coordinate" \
                   "\n y() : returns the y value of the Coordinate" \
                   "\n z() : returns the z value of the Coordinate"
                   "\n\n SETTER: " \
                   "\n setx(x) : sets the x value of the Coordinate, expects float " \
                   "\n sety(y) : sets the y value of the Coordinate, expects float " \
                   "\n setz(z) : sets the z value of the Coordinate, excpets float "
                   );
}

void FloatCoordinateDecorator::setx(floatCoordinate *self, float x) {
    self->x = x;
}

void FloatCoordinateDecorator::sety(floatCoordinate *self, float y) {
    self->y = y;
}

void FloatCoordinateDecorator::setz(floatCoordinate *self, float z) {
    self->z = z;
}





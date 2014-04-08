#include "floatcoordinatedecorator.h"
#include "knossos-global.h"
#include <QDebug>




FloatCoordinateDecorator::FloatCoordinateDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<FloatCoordinate>();
}

FloatCoordinate *FloatCoordinateDecorator::new_FloatCoordinate() {
    return new FloatCoordinate();
}

FloatCoordinate *FloatCoordinateDecorator::new_FloatCoordinate(float x, float y, float z) {
    return new FloatCoordinate(x, y, z);
}

float FloatCoordinateDecorator::x(FloatCoordinate *self) {
    return self->x;
}

float FloatCoordinateDecorator::y(FloatCoordinate *self) {
    return self->y;
}

float FloatCoordinateDecorator::z(FloatCoordinate *self) {
    return self->z;
}

QString FloatCoordinateDecorator::static_FloatCoordinate_help() {
    return QString("An instanceable class storing 3D coordinates as float. Access to attributes only via getter and setter." \
                   "\n\n CONSTRUCTORS: " \
                   "\n FloatCoordinate() : creates an empty FloatCoordinate object. " \
                   "\n FloatCoordinate(x, y, z) : creates a FloatCoordinates object where all dimensions are to specify." \
                   "\n\n GETTER: " \
                   "\n x() : returns the x value of the coordinate" \
                   "\n y() : returns the y value of the coordinate" \
                   "\n z() : returns the z value of the coordinate"
                   "\n\n SETTER: " \
                   "\n setx(x) : sets the x value of the coordinate, expects float " \
                   "\n sety(y) : sets the y value of the coordinate, expects float " \
                   "\n setz(z) : sets the z value of the coordinate, excpets float "
                   );
}

void FloatCoordinateDecorator::setx(FloatCoordinate *self, float x) {
    self->x = x;
}

void FloatCoordinateDecorator::sety(FloatCoordinate *self, float y) {
    self->y = y;
}

void FloatCoordinateDecorator::setz(FloatCoordinate *self, float z) {
    self->z = z;
}





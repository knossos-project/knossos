/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

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

QVector<int> CoordinateDecorator::vector(Coordinate *self) {
    return self->vector();
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

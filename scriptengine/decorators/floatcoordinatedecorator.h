/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#ifndef FloatCoordinateDECORATOR_H
#define FloatCoordinateDECORATOR_H

#include "coordinate.h"

#include <QObject>

class FloatCoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit FloatCoordinateDecorator(QObject *parent = nullptr);

signals:

public slots:
    floatCoordinate *new_floatCoordinate();
    floatCoordinate *new_floatCoordinate(float x, float y, float z);
    float x(floatCoordinate *self);
    float y(floatCoordinate *self);
    float z(floatCoordinate *self);

    void setx(floatCoordinate *self, float x);
    void sety(floatCoordinate *self, float y);
    void setz(floatCoordinate *self, float z);

    QString static_floatCoordinate_help();


};

#endif // FloatCoordinateDECORATOR_H

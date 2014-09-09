#ifndef COORDINATEDECORATOR_H
#define COORDINATEDECORATOR_H

#include "coordinate.h"

#include <QObject>

class CoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit CoordinateDecorator(QObject *parent = 0);


signals:

public slots:

    int x(Coordinate *self);
    int y(Coordinate *self);
    int z(Coordinate *self);

    QString static_Coordinate_help();


};

#endif // COORDINATEDECORATOR_H

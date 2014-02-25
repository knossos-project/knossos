#ifndef COORDINATEDECORATOR_H
#define COORDINATEDECORATOR_H

#include <QObject>

class Coordinate;
class CoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit CoordinateDecorator(QObject *parent = 0);
    Coordinate *new_Coordinate();
    Coordinate *new_Coordinate(int x, int y, int z);
    int getX(Coordinate *self);
    int getY(Coordinate *self);
    int getZ(Coordinate *self);
    void setX(Coordinate *self, int x);
    void setY(Coordinate *self, int y);
    void setZ(Coordinate *self, int z);


signals:

public slots:

};

#endif // COORDINATEDECORATOR_H

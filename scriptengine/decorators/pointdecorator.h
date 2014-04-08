#ifndef POINTDECORATOR_H
#define POINTDECORATOR_H

#include <QObject>

class Color4F;
class Coordinate;
class Point;
class Transform;
class PointDecorator : public QObject
{
    Q_OBJECT
public:
    explicit PointDecorator(QObject *parent = 0);

signals:

public slots:
   Point *new_Point(Transform *transform = 0, Coordinate *origin = 0, uint size = 0, Color4F *color = 0);
};

#endif // POINTDECORATOR_H

#ifndef TRANSFORMDECORATOR_H
#define TRANSFORMDECORATOR_H

#include "coordinate.h"

#include <QObject>

class Transform;
class TransformDecorator : public QObject
{
    Q_OBJECT
public:
    TransformDecorator();

public slots:
    Transform *new_Transform(Coordinate *translate = 0, Coordinate *rotate = 0, Coordinate *scale = 0);
};

#endif // TRANSFORMDECORATOR_H

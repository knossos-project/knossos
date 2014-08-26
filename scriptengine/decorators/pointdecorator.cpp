#include "pointdecorator.h"
#include "knossos-global.h"
#include "../geometry/point.h"


PointDecorator::PointDecorator(QObject *parent) :
    QObject(parent)
{
}



Point *PointDecorator::new_Point(Transform *transform, Coordinate *origin, uint size, color4F *color) {

}

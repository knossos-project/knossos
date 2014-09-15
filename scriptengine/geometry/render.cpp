#include "render.h"
#include "../geometry/point.h"

Render::Render(QObject *parent) :
    QObject(parent)
{
    userGeometry = new QList<Shape *>();
}

void Render::addPoint(Point *) {
}

#include "colordecorator.h"
#include "knossos-global.h"

ColorDecorator::ColorDecorator(QObject *parent) :
    QObject(parent)
{
}

Color *ColorDecorator::new_Color() {
    return new Color();
}

Color *ColorDecorator::new_Color(float r, float g, float b, float a) {
    return new Color(r, g, b, a);
}

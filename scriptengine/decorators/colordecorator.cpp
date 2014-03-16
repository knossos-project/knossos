#include "colordecorator.h"
#include "knossos-global.h"

ColorDecorator::ColorDecorator(QObject *parent) :
    QObject(parent)
{
}

/*
color4F *ColorDecorator::new_color4F() {
    return new color4F();
}

color4F *ColorDecorator::new_color4F(float r, float g, float b, float a) {
    return new color4F(r, g, b, a);
}
*/

float ColorDecorator::red(color4F *self) {
    return self->r;
}

float ColorDecorator::green(color4F *self) {
    return self->g;
}

float ColorDecorator::blue(color4F *self) {
    return self->b;
}

float ColorDecorator::alpha(color4F *self) {
    return self->a;
}

/*
void ColorDecorator::set_red(color4F *self, float red) {
    self->r = red;
}

void ColorDecorator::set_green(color4F *self, float green) {
    self->g = green;
}

void ColorDecorator::set_blue(color4F *self, float blue) {
    self->r = blue;
}

void ColorDecorator::set_alpha(color4F *self, float alpha) {
    self->a = alpha;
}
*/



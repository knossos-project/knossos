#include "colordecorator.h"
#include "knossos-global.h"

Q_DECLARE_METATYPE(color4F)

ColorDecorator::ColorDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<color4F>();
}


color4F *ColorDecorator::new_color4F() {
    return new color4F();
}

color4F *ColorDecorator::new_color4F(float r, float g, float b, float a) {
    return new color4F(r, g, b, a);
}

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

QString ColorDecorator::static_color4F_help() {
    return QString("An instanteable class which stores rgba channels in float coordinates in range [0..1]. Access to attributes only via getter and setter." \
                   "\n\n CONSTRUCTORS: " \
                   "\n Color() : creates an empty color object." \
                   "\n Color(red, green, blue, alpha) : creates a color object by specifying all color channels. Excepts whether float values in range [0..1]"
                   "\n\n GETTER: " \
                   "\n red() : returns the red channel of the color" \
                   "\n green() : returns the green channel of the color" \
                   "\n blue() : returns the blue channel of the color" \
                   "\n alpha() : returns the alpha channel of the color" \
                   "\n\n SETTER: " \
                   "\n set_red(red) : sets the red channel of the color. Expects a float value in range [0..1]" \
                   "\n set_green(green) : sets the green channel of the color. Expects a float value in range [0..1] " \
                   "\n set_blue(blue) : sets the blue channel of the color. Expects a float value in range [0..1]" \
                   "\n set_alpha(alpha) : sets the alpha channel of the color. Expects a floats value in range [0..1] ");
}


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

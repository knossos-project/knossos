#ifndef Color4FDECORATOR_H
#define Color4FDECORATOR_H

#include <QObject>

class Color4F;
class ColorDecorator : public QObject
{
    Q_OBJECT
public:
    explicit ColorDecorator(QObject *parent = 0);

    
signals:
    
public slots:

    Color4F *new_Color4F();
    Color4F *new_Color4F(float r, float g, float b, float a = 1.0);

    float red(Color4F *self);
    float blue(Color4F *self);
    float green(Color4F *self);
    float alpha(Color4F *self);

    void set_red(Color4F *self, float red);
    void set_green(Color4F *self, float green);
    void set_blue(Color4F *self, float blue);
    void set_alpha(Color4F *self, float alpha);

    QString static_Color4F_help();
};

#endif // Color4FDECORATOR_H

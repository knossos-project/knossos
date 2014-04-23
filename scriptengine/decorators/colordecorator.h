#ifndef Color4FDECORATOR_H
#define Color4FDECORATOR_H

#include <QObject>

class color4F;
class ColorDecorator : public QObject
{
    Q_OBJECT
public:
    explicit ColorDecorator(QObject *parent = 0);

    
signals:
    
public slots:

    color4F *new_Color4F();
    color4F *new_Color4F(float r, float g, float b, float a = 1.0);

    float red(color4F *self);
    float blue(color4F *self);
    float green(color4F *self);
    float alpha(color4F *self);

    void set_red(color4F *self, float red);
    void set_green(color4F *self, float green);
    void set_blue(color4F *self, float blue);
    void set_alpha(color4F *self, float alpha);

    QString static_Color4F_help();
};

#endif // Color4FDECORATOR_H

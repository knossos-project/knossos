#ifndef COLOR4FDECORATOR_H
#define COLOR4FDECORATOR_H

#include <QObject>

class Color;
class ColorDecorator : public QObject
{
    Q_OBJECT
public:
    explicit ColorDecorator(QObject *parent = 0);

    
signals:
    
public slots:
    Color *new_Color();
    Color *new_Color(float r, float g, float b, float a = 1.0);
};

#endif // COLOR4FDECORATOR_H

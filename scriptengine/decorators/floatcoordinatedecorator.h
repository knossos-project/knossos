#ifndef FLOATCOORDINATEDECORATOR_H
#define FLOATCOORDINATEDECORATOR_H

#include <QObject>

class FloatCoordinate;
class FloatCoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit FloatCoordinateDecorator(QObject *parent = 0);    

signals:

public slots:
    FloatCoordinate *new_FloatCoordinate();
    FloatCoordinate *new_FloatCoordinate(float x, float y, float z);
    float x(FloatCoordinate *self);
    float y(FloatCoordinate *self);
    float z(FloatCoordinate *self);

    void setx(FloatCoordinate *self, float x);
    void sety(FloatCoordinate *self, float y);
    void setz(FloatCoordinate *self, float z);

    QString static_FloatCoordinate_help();


};

#endif // FLOATCOORDINATEDECORATOR_H

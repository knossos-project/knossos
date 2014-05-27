#ifndef FloatCoordinateDECORATOR_H
#define FloatCoordinateDECORATOR_H

#include <QObject>

class floatCoordinate;
class FloatCoordinateDecorator : public QObject
{
    Q_OBJECT
public:
    explicit FloatCoordinateDecorator(QObject *parent = 0);    

signals:

public slots:
    floatCoordinate *new_floatCoordinate();
    floatCoordinate *new_floatCoordinate(float x, float y, float z);
    float x(floatCoordinate *self);
    float y(floatCoordinate *self);
    float z(floatCoordinate *self);

    void setx(floatCoordinate *self, float x);
    void sety(floatCoordinate *self, float y);
    void setz(floatCoordinate *self, float z);

    QString static_floatCoordinate_help();


};

#endif // FloatCoordinateDECORATOR_H

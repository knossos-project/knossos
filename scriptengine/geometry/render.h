#ifndef RENDER_H
#define RENDER_H

#include <QObject>
#include <QList>

class Point;
class Shape;
class Render : public QObject
{
    Q_OBJECT
public:
    explicit Render(QObject *parent = 0);
    QList<Shape *> *userGeometry;
signals:

public slots:
    void addPoint(Point *point);
};

#endif // RENDER_H

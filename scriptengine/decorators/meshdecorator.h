#ifndef MESHDECORATOR_H
#define MESHDECORATOR_H

#include <QObject>
#include <QList>

class mesh;
class color4F;
class floatCoordinate;
class MeshDecorator : public QObject
{
    Q_OBJECT
public:
    explicit MeshDecorator(QObject *parent = 0);

signals:

public slots:
    mesh *new_mesh(uint mode);
    QList<floatCoordinate *> *vertices(mesh *self);
    QList<floatCoordinate *> *normals(mesh *self);
    QList<color4F *> *colors(mesh *self);

    void set_vertices(mesh *self, QList<floatCoordinate *> *vertices);
    void set_normals (mesh *self, QList<floatCoordinate *> *normals);
    void set_colors (mesh *self, QList<color4F *> *vertices);

    /*
    uint vertsBuffSize;
    uint normsBuffSize;
    uint colsBuffSize;
    // indicates last used element in corresponding buffer
    uint vertsIndex;
    uint normsIndex;
    uint colsIndex;
    */

};

#endif // MESHDECORATOR_H

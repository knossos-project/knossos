#ifndef MESHDECORATOR_H
#define MESHDECORATOR_H

#include <QObject>
#include <QList>
#include "knossos-global.h"

class MeshDecorator : public QObject
{
    Q_OBJECT
public:
    explicit MeshDecorator(QObject *parent = 0);

signals:

public slots:
    Mesh *new_Mesh(uint mode);
    QList<FloatCoordinate *> *vertices(Mesh *self);
    QList<FloatCoordinate *> *normals(Mesh *self);
    QList<Color4F *> *colors(Mesh *self);

    void set_vertices(Mesh *self, QList<QVariant> &vertices);
    void set_normals (Mesh *self, QList<QVariant> &normals);
    void set_colors (Mesh *self, QList<QVariant> &vertices);
    void set_size(Mesh *self, uint size);

    QString static_Mesh_help();


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

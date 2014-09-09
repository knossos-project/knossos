#ifndef MeshDECORATOR_H
#define MeshDECORATOR_H

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
    QList<floatCoordinate *> *vertices(mesh *self);
    QList<floatCoordinate *> *normals(mesh *self);
    QList<color4F *> *colors(mesh *self);

    void set_vertices(mesh *self, QList<QVariant> &vertices);
    void set_normals (mesh *self, QList<QVariant> &normals);
    void set_colors (mesh *self, QList<QVariant> &vertices);
    void set_size(mesh *self, uint size);

    QString static_mesh_help();
};

#endif // MeshDECORATOR_H

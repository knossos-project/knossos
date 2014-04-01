#include "meshdecorator.h"
#include "knossos-global.h"
#include "renderer.h"
#include <QDebug>

Q_DECLARE_METATYPE(color4F)
Q_DECLARE_METATYPE(mesh)



MeshDecorator::MeshDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<mesh>();




}

mesh *MeshDecorator::new_mesh(uint mode) {
    mesh *instance = new mesh(mode);
    Renderer::initMesh(instance, 2);
    return instance;
}

QList<floatCoordinate *> *MeshDecorator::vertices(mesh *self) {
    QList<floatCoordinate *> *vertices = new QList<floatCoordinate *>();
    for(int i = 0; i < self->vertsIndex; i++) {
        floatCoordinate *currentVertex = self->vertices++;
        vertices->append(currentVertex);
    }

    return vertices;
}

QList<floatCoordinate *> *MeshDecorator::normals(mesh *self) {
    QList<floatCoordinate *> *normals = new QList<floatCoordinate *>();
    for(int i = 0; i < self->normsIndex; i++) {
        floatCoordinate *currentNormal = self->normals++;
        normals->append(currentNormal);
    }

    return normals;
}


QList<color4F *> *MeshDecorator::colors(mesh *self) {
    QList<color4F *> *colors = new QList<color4F *>();
    for(int i = 0; i < self->colsIndex; i++) {
        color4F *currentColor = self->colors++;
        colors->append(currentColor);
    }

    return colors;
}

void MeshDecorator::set_vertices(mesh *self, QList<QVariant> &vertices) {
    Renderer::resizeMeshCapacity(self, vertices.size());

    for(int i = 0; i < vertices.size(); i++) {
        if(vertices.at(i).canConvert<floatCoordinate>()) {
            self->vertices[i] = vertices.at(i).value<floatCoordinate>();
        }
    }

    self->vertsIndex = vertices.size();
}

void MeshDecorator::set_normals(mesh *self, QList<QVariant> &normals) {

    for(int i = 0; i < normals.size(); i++) {
        if(normals.at(i).canConvert<floatCoordinate>()) {
            self->normals[i] = normals.at(i).value<floatCoordinate>();
        }
    }


    self->normsIndex = normals.size();

}

void MeshDecorator::set_colors(mesh *self, QList<QVariant> &colors) {

    for(int i = 0; i < colors.size(); i++) {
        self->colors[i] = colors.at(i).value<color4F>();
    }

    self->colsIndex = colors.size();

}

void MeshDecorator::set_size(mesh *self, uint size) {
    self->size = size;
}

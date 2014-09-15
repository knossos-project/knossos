#include "meshdecorator.h"
#include "knossos-global.h"
#include "viewer.h"
#include "renderer.h"
#include <QDebug>

Q_DECLARE_METATYPE(color4F)

Q_DECLARE_METATYPE(mesh)

MeshDecorator::MeshDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<mesh>();

}

QList<floatCoordinate *> *MeshDecorator::vertices(mesh *self) {
    QList<floatCoordinate *> *vertices = new QList<floatCoordinate *>();
    for (uint i = 0; i < self->vertsIndex; i++) {
        floatCoordinate *currentVertex = self->vertices++;
        vertices->append(currentVertex);
    }

    return vertices;
}

QList<floatCoordinate *> *MeshDecorator::normals(mesh *self) {
    QList<floatCoordinate *> *normals = new QList<floatCoordinate *>();
    for (uint i = 0; i < self->normsIndex; i++) {
        floatCoordinate *currentNormal = self->normals++;
        normals->append(currentNormal);
    }

    return normals;
}


QList<color4F *> *MeshDecorator::colors(mesh *self) {
    QList<color4F *> *colors = new QList<color4F *>();
    for (uint i = 0; i < self->colsIndex; i++) {
        color4F *currentColor = self->colors++;
        colors->append(currentColor);
    }

    return colors;
}

void MeshDecorator::set_vertices(mesh *self, QList<QVariant> &vertices) {
    state->viewer->renderer->resizemeshCapacity(self, vertices.size());

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

/** @todo check the size-commands for the other modes */
QString MeshDecorator::static_mesh_help() {
    return QString("An instanceable class for rendering geometry from python. Access to the attributes only via getter and setter." \
                   "\n\n CONSTRUCTORS:" \
                   "\n mesh(mode) : Creates a mesh object. The constructor expects an openGL vertex mode constant:" \
                   "\n\t GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS, GL_POLYGON" \
                   "\n\n GETTER: " \
                   "\n vertices() : returns a list of float coordinates of the current mesh" \
                   "\n normals() : returns a list of float coordinates with normal coordinates" \
                   "\n colors() : returns a list of colors" \
                   "\n\n SETTER: " \
                   "\n set_vertices(vertexList) : excepts a list of FloatCoordinates representing vertices" \
                   "\n set_normals(normalList) : excepts a list of FloatCoordinates repesentation normal vectors" \
                   "\n set_colors(colorList) : excepts a list of color4F objects" \
                   "\n set_size(size) : sets the specific size of the geometry: " \
                   "\n if mode is GL_POINTS then size determines the point size." \
                   "\n if mode is GL_LINES then size determines the lines width." \
                   "");

}

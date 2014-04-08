#include "meshdecorator.h"
#include "knossos-global.h"
#include "renderer.h"
#include <QDebug>

Q_DECLARE_METATYPE(Color4F)
Q_DECLARE_METATYPE(Mesh)



MeshDecorator::MeshDecorator(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<Mesh>();

}

Mesh *MeshDecorator::new_Mesh(uint mode) {
    Mesh *instance = new Mesh(mode);
    Renderer::initMesh(instance, 2);
    return instance;
}

QList<FloatCoordinate *> *MeshDecorator::vertices(Mesh *self) {
    QList<FloatCoordinate *> *vertices = new QList<FloatCoordinate *>();
    for(int i = 0; i < self->vertsIndex; i++) {
        FloatCoordinate *currentVertex = self->vertices++;
        vertices->append(currentVertex);
    }

    return vertices;
}

QList<FloatCoordinate *> *MeshDecorator::normals(Mesh *self) {
    QList<FloatCoordinate *> *normals = new QList<FloatCoordinate *>();
    for(int i = 0; i < self->normsIndex; i++) {
        FloatCoordinate *currentNormal = self->normals++;
        normals->append(currentNormal);
    }

    return normals;
}


QList<Color4F *> *MeshDecorator::colors(Mesh *self) {
    QList<Color4F *> *colors = new QList<Color4F *>();
    for(int i = 0; i < self->colsIndex; i++) {
        Color4F *currentColor = self->colors++;
        colors->append(currentColor);
    }

    return colors;
}

void MeshDecorator::set_vertices(Mesh *self, QList<QVariant> &vertices) {
    Renderer::resizeMeshCapacity(self, vertices.size());

    for(int i = 0; i < vertices.size(); i++) {
        if(vertices.at(i).canConvert<FloatCoordinate>()) {
            self->vertices[i] = vertices.at(i).value<FloatCoordinate>();
        }
    }

    self->vertsIndex = vertices.size();
}

void MeshDecorator::set_normals(Mesh *self, QList<QVariant> &normals) {

    for(int i = 0; i < normals.size(); i++) {
        if(normals.at(i).canConvert<FloatCoordinate>()) {
            self->normals[i] = normals.at(i).value<FloatCoordinate>();
        }
    }


    self->normsIndex = normals.size();

}

void MeshDecorator::set_colors(Mesh *self, QList<QVariant> &colors) {

    for(int i = 0; i < colors.size(); i++) {
        self->colors[i] = colors.at(i).value<Color4F>();
    }

    self->colsIndex = colors.size();

}

void MeshDecorator::set_size(Mesh *self, uint size) {
    self->size = size;
}

/** @todo check the size-commands for the other modes */
QString MeshDecorator::static_Mesh_help() {
    return QString("An instanceable class for rendering geometry from python. Access to the attributes only via getter and setter." \
                   "\n\n CONSTRUCTORS:" \
                   "\n Mesh(mode) : Creates a Mesh object. The constructor expects an openGL vertex mode constant:" \
                   "\n\t GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS, GL_POLYGON" \
                   "\n\n GETTER: " \
                   "\n vertices() : returns a list of float coordinates of the current Mesh" \
                   "\n normals() : returns a list of float coordinates with normal coordinates" \
                   "\n colors() : returns a list of colors" \
                   "\n\n SETTER: " \
                   "\n set_vertices(vertexList) : excepts a list of FloatCoordinates representing vertices" \
                   "\n set_normals(normalList) : excepts a list of FloatCoordinates repesentation normal vectors" \
                   "\n set_colors(colorList) : excepts a list of Color4F objects" \
                   "\n set_size(size) : sets the specific size of the geometry: " \
                   "\n if mode is GL_POINTS then size determines the point size." \
                   "\n if mode is GL_LINES then size determines the lines width." \
                   "");

}

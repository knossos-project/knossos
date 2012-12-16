#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>

#include "eventmodel.h"

enum VIEWPORT {
    XY, XZ, YZ, SKELETON // 1 XY, 2 XZ, 3 YZ, 4 SKELETON
};

class Viewport : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent, int plane);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    int plane;
    EventModel *eventModel;

signals:
    
public slots:
    
};

#endif // VIEWPORT_H

#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>
#include <iostream>
//#include "eventmodel.h"
using namespace std;


class OpenGLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget *parent, int viewPort);

    void initializeGL();
    void resizeGL(int w, int h);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void paintGL();


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
    int viewPort;
    bool *controls; // [0] for CTRL, [1] for SHIT, [2] for ALT
    //EventModel *eventModel;
signals:
    
public slots:
    
};

#endif // OPENGLWIDGET_H

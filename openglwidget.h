#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>
//#include "eventmodel.h"



class OpenGLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget *parent, int viewPort);

protected:
    int viewPort;
    bool *controls; // [0] for CTRL, [1] for SHIT, [2] for ALT
    //EventModel *eventModel;
signals:
    
public slots:
    
};

#endif // OPENGLWIDGET_H

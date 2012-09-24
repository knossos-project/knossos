#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>



class OpenGLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit OpenGLWidget(QWidget *parent = 0);

protected:


signals:
    
public slots:
    
};

#endif // OPENGLWIDGET_H

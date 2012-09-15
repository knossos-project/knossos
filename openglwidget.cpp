#include "openglwidget.h"

OpenGLWidget::OpenGLWidget(QWidget *parent) :
    QGLWidget(parent)
{
}

void OpenGLWidget::initializeGL()
{
    glClearColor(0, 0, 0, 1);
}

void OpenGLWidget::resizeGL(int w, int h)
{

}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{

}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{

}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{

}

#include "openglwidget.h"


OpenGLWidget::OpenGLWidget(QWidget *parent, int viewPort) :
    QGLWidget(parent)
{
    this->viewPort = viewPort;
    eventModel = new EventModel();
    /* per default the widget only receives move event when at least one mouse button is pressed
    to change this behaviour we need to track the mouse position */
    this->setMouseTracking(true);
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

    //qDebug() << event->x() << " " << qDebug() << event->y();

}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        eventModel->handleMouseButtonLeft(event, viewPort);
    } else if(event->button() == Qt::MiddleButton) {
        eventModel->handleMouseButtonMiddle(event, viewPort);
    } else if(event->button() == Qt::RightButton) {
        eventModel->handleMouseButtonRight(event, viewPort);
    }

}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{



}

void OpenGLWidget::wheelEvent(QWheelEvent *event) {
    if(event->delta() > 0) {
        eventModel->handleMouseWheelForward(event, viewPort);
    } else {
        eventModel->handleMouseWheelBackward(event, viewPort);
    }
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{

    eventModel->handleKeyboard(event);
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{

}


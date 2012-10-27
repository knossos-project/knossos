#include "openglwidget.h"


OpenGLWidget::OpenGLWidget(QWidget *parent, int viewPort) :
    QGLWidget(parent)
{
    this->viewPort = viewPort;
    this->controls = new bool[3];
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
        eventModel->handleMouseButtonLeft(event, viewPort, controls);
    } else if(event->button() == Qt::MiddleButton) {
        eventModel->handleMouseButtonMiddle(event, viewPort, controls);
    } else if(event->button() == Qt::RightButton) {
        eventModel->handleMouseButtonRight(event, viewPort, controls);
    }

}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // TODO


}

void OpenGLWidget::wheelEvent(QWheelEvent *event) {
    if(event->delta() > 0) {
        eventModel->handleMouseWheelForward(event, viewPort, controls);
    } else {
        eventModel->handleMouseWheelBackward(event, viewPort, controls);
    }
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Control) {
        controls[0] = true;
    }
    if(event->key() == Qt::Key_Shift) {
        controls[1] = true;
    }
    if(event->key() == Qt::Key_Alt) {
        controls[2] = true;
    }

    eventModel->handleKeyboard(event);
}

void OpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Control) {
        controls[0] = false;
    }
    if(event->key() == Qt::Key_Shift) {
        controls[1] = false;
    }
    if(event->key() == Qt::Key_Alt) {
        controls[2] = false;
    }
}


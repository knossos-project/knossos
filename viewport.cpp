#include "viewport.h"


Viewport::Viewport(QWidget *parent, int plane) :
    QGLWidget(parent)
{
    this->plane = plane;
    eventModel = new EventModel();
    /* per default the widget only receives move event when at least one mouse button is pressed
    to change this behaviour we need to track the mouse position */
    this->setMouseTracking(true);
    setStyleSheet("background:black");
    this->setCursor(Qt::CrossCursor);

}

void Viewport::initializeGL()
{
    glClearColor(0, 0, 0, 1);
    /* display some basic openGL driver statistics */
    qDebug("OpenGL v%s on %s from %s\n", glGetString(GL_VERSION),
    glGetString(GL_RENDERER), glGetString(GL_VENDOR));

}

void Viewport::resizeGL(int w, int h)
{

}

void Viewport::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{

    //qDebug() << event->x() << " " << qDebug() << event->y();

}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        eventModel->handleMouseButtonLeft(event, plane);
    } else if(event->button() == Qt::MiddleButton) {
        eventModel->handleMouseButtonMiddle(event, plane);
    } else if(event->button() == Qt::RightButton) {
        eventModel->handleMouseButtonRight(event, plane);
    }

}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{



}

void Viewport::wheelEvent(QWheelEvent *event) {
    if(event->delta() > 0) {
        eventModel->handleMouseWheelForward(event, plane);
    } else {
        eventModel->handleMouseWheelBackward(event, plane);
    }
}

void Viewport::keyPressEvent(QKeyEvent *event)
{

    eventModel->handleKeyboard(event);
}

void Viewport::keyReleaseEvent(QKeyEvent *event)
{

}

void Viewport::customEvent(QEvent *event) {
    if(event->type() == QEvent::User) {

    }
}


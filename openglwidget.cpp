#include "openglwidget.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

OpenGLWidget::OpenGLWidget(QWidget *parent) :
    QGLWidget(parent)
{
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
    qDebug() << event->x() << " " << qDebug() << event->y();

}

void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {

    } else if(event->button() == Qt::MiddleButton) {

    } else if(event->button() == Qt::RightButton) {

    }

}

void OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{

    for(i = 0; i < state->viewerState->numberViewPorts; i++) {
        state->viewerState->viewPorts[i].draggedNode = NULL;
        state->viewerState->viewPorts[i].motionTracking = FALSE;
        state->viewerState->viewPorts[i].VPmoves = FALSE;
        state->viewerState->viewPorts[i].VPresizes = FALSE;
        state->viewerState->viewPorts[i].userMouseSlideX = 0.;
        state->viewerState->viewPorts[i].userMouseSlideY = 0.;
    }

}

void OpenGLWidget::wheelEvent(QWheelEvent *event) {

}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{

}



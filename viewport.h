#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL>
#include <QDebug>

#include "eventmodel.h"

class QPushButton;
class Viewport : public QGLWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent, int plane);

    void drawViewport(int plane);
    void drawSkeletonViewport();

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
    void customEvent(QEvent *event);
    int xrel(int x);
    int yrel(int y);
    int plane; //XY_VIEWPORT, ...
    int lastX; //last x position
    int lastY; //last y position
    QPushButton *xy, *xz, *yz, *flip, *reset;

private:
    bool handleMouseButtonLeft(QMouseEvent *event, int32_t VPfound);
    bool handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound);
    bool handleMouseButtonRight(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotion(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int32_t VPfound);
    bool handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound);
    bool handleKeyboard(QKeyEvent *event);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound);

signals:
    void mouseDrag(QEvent *event, int plane);
public slots:
    
};

#endif // VIEWPORT_H

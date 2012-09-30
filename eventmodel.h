#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <stdint.h>
#include <math.h>
#include "../knossos-global.h"

class EventModel : public QObject
{
    Q_OBJECT
public:
    explicit EventModel(QObject *parent = 0);
    bool handleMouseButtonLeft(QMouseEvent *event, int32_t VPfound, bool *controls);
    bool handleMouseButtonMiddle(QMouseEvent *event, int32_t VPfound, bool *controls);
    bool handleMouseButtonRight(QMouseEvent *event, int32_t VPfound, bool *controls);
    bool handleMouseMotion(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int32_t VPfound);
    bool handleMouseWheelForward(QWheelEvent *event, int32_t VPfound, bool *controls);
    bool handleMouseWheelBackward(QWheelEvent *event, int32_t VPfound, bool *controls);
    bool handleKeyboard(QKeyEvent *event);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound);

    int xrel(int x);
    int yrel(int y);

protected:
    int mouseX;
    int mouseY;

signals:
    
public slots:
    
};

#endif // EVENTMODEL_H

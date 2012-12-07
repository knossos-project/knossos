#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <stdint.h>
#include <math.h>

extern "C" {
    #include "knossos-global.h"
}

#include "renderer.h"
#include "skeletonizer.h"
#include "knossos.h"
#include "viewer.h"
#include "remote.h"
#include "eventmodel.h"

/**
  * @class EventModel
  * @brief This is the eventHandler from Knossos 3.2 adjusted for the QT version
  *
  * The class adopts the core functionality from eventHandler with the exception that
  * SDL Events are replaced through the corresponding QT-Events
  */
class EventModel : public QObject
{
    Q_OBJECT
public:
    static bool handleEvent(SDL_Event event);
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

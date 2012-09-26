#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include "../knossos-global.h"

class EventModel : public QObject
{
    Q_OBJECT
public:
    explicit EventModel(QObject *parent = 0);
    bool handleMouseButtonLeft(QMouseEvent *event, int32_t VPFound, bool *controls);
    bool handleMouseButtonMiddle(QMouseEvent *event, int32_t VPFound, bool controls);
    bool handleMouseButtonRight(QMouseEvent *event, int32_t VPFound, bool *controls);
    bool handleMouseMotion(QMouseEvent *event, int32_t VPFound);
    bool handleMouseMotionLeftHold(QMouseEvent *event, int32_t VPFound);
    bool handleMouseMotionMiddleHold(QMouseEvent *event, int32_t VPFound);
    bool handleMouseMotionRightHold(QMouseEvent *event, int32_t VPFound);
    bool handleMouseWheelForward(QWheelEvent *event, int32_t VPFound, bool *controls);
    bool handleMouseWheelBackward(QWheelEvent *event, int32_t VPFound, bool *controls);
    bool handleKeyboard(QKeyEvent *event);
    static Coordinate *getCoordinateFromOrthogonalClick(QMouseEvent *event, int32_t VPfound);

signals:
    
public slots:
    
};

#endif // EVENTMODEL_H

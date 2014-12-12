/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include "session.h"

#include <QApplication>

class Session::ActivityEventFilter : public QObject {
    bool & timeSliceActivity;
public:
    ActivityEventFilter(Session & session) : timeSliceActivity(session.timeSliceActivity) {}
    bool eventFilter(QObject *object, QEvent *event) {
        // mark time slice as valid on any user actions except just moving the mouse
        int type = event->type();
        if (type == QEvent::MouseButtonPress || type == QEvent::KeyPress || type == QEvent::Wheel) {
             timeSliceActivity = true;
        }
        return QObject::eventFilter(object, event);
    }
};

Session::Session() {
    qApp->installEventFilter(new ActivityEventFilter(*this));

    annotationTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&annotationTimer, &QTimer::timeout, this, &Session::handleTimeSlice);
    annotationTimer.start(TIME_SLICE_MS);

    lastTimeSlice.start();
}

bool Session::outsideMovementArea(Coordinate pos) {
    return pos.x < movementCenter.x - movementRange.x || pos.x > movementCenter.x + movementRange.x ||
           pos.y < movementCenter.y - movementRange.y || pos.y > movementCenter.y + movementRange.y ||
           pos.z < movementCenter.z - movementRange.z || pos.z > movementCenter.z + movementRange.z;
}

void Session::updateMovementArea(Coordinate center, Coordinate range) {
    movementCenter = center;
    movementRange = range;
    emit movementAreaChanged();
}

decltype(Session::annotationTimeMilliseconds) Session::annotationTime() const {
    return annotationTimeMilliseconds;
}

void Session::annotationTime(const decltype(annotationTimeMilliseconds) & ms) {
    annotationTimeMilliseconds = ms;

    const auto absoluteMinutes = annotationTimeMilliseconds / 1000 / 60;
    const auto hours = absoluteMinutes / 60;
    const auto minutes = absoluteMinutes % 60;

    emit annotationTimeChanged(QString("%1:%2h annotation time").arg(hours).arg(minutes, 2, 10, QChar('0')));
}

decltype(Session::annotationTimeMilliseconds) Session::currentTimeSliceMs() const {
    return lastTimeSlice.elapsed();
}

void Session::handleTimeSlice() {
    if (timeSliceActivity) {
        annotationTime(annotationTimeMilliseconds + TIME_SLICE_MS);
        timeSliceActivity = false;
    }
    lastTimeSlice.restart();
}

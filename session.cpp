/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *
 *
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "session.h"

#include "dataset.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"

#include <QApplication>

class Session::ActivityEventFilter : public QObject {
    bool & timeSliceActivity;
public:
    ActivityEventFilter(Session & session) : QObject(&session), timeSliceActivity(session.timeSliceActivity) {}
    virtual bool eventFilter(QObject *object, QEvent *event) override {
        // mark time slice as valid on any user actions except just moving the mouse
        int type = event->type();
        if (type == QEvent::MouseButtonPress || type == QEvent::KeyPress || type == QEvent::Wheel) {
             timeSliceActivity = true;
        }
        return QObject::eventFilter(object, event);
    }
};

Session::Session() : annotationMode(AnnotationMode::Mode_Tracing) {
    qApp->installEventFilter(new ActivityEventFilter(*this));

    annotationTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&annotationTimer, &QTimer::timeout, this, &Session::handleTimeSlice);
    annotationTimer.start(TIME_SLICE_MS);
    lastTimeSlice.start();

    autoSaveTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&autoSaveTimer, &QTimer::timeout, [this]() {
       if(unsavedChanges) {
           emit autoSaveSignal();
       }
    });
}

void Session::clearAnnotation() {
    Skeletonizer::singleton().clearSkeleton();
    Segmentation::singleton().clear();
    resetMovementArea();
    setAnnotationTime(0);
    annotationFilename = "";
    extraFiles.clear();
    emit clearedAnnotation();
    unsavedChanges = false;
}

bool Session::outsideMovementArea(const Coordinate & pos) {
    return pos.x < movementAreaMin.x || pos.x >= movementAreaMax.x ||
           pos.y < movementAreaMin.y || pos.y >= movementAreaMax.y ||
           pos.z < movementAreaMin.z || pos.z >= movementAreaMax.z;
}

void Session::updateMovementArea(const Coordinate & min, const Coordinate & max) {
    movementAreaMin = min.capped({0, 0, 0}, Dataset::current().boundary);
    movementAreaMax = max.capped({0, 0, 0}, Dataset::current().boundary);
    // fixup empty movement area, ignore edge case at boundary
    const auto size = movementAreaMax - movementAreaMin;
    movementAreaMax.x = size.x < 1 ? movementAreaMin.x + 1 : movementAreaMax.x;
    movementAreaMax.y = size.y < 1 ? movementAreaMin.y + 1 : movementAreaMax.y;
    movementAreaMax.z = size.z < 1 ? movementAreaMin.z + 1 : movementAreaMax.z;
    emit movementAreaChanged();
}

void Session::resetMovementArea() {
    updateMovementArea({0, 0, 0}, Dataset::current().boundary);
}

decltype(Session::annotationTimeMilliseconds) Session::getAnnotationTime() const {
    return annotationTimeMilliseconds;
}

void Session::setAnnotationTime(const decltype(annotationTimeMilliseconds) & ms) {
    annotationTimeMilliseconds = ms;

    const auto absoluteMinutes = annotationTimeMilliseconds / 1000 / 60;
    const auto hours = absoluteMinutes / 60;
    const auto minutes = absoluteMinutes % 60;

    emit annotationTimeChanged(QString("%1:%2 h annotation time").arg(hours).arg(minutes, 2, 10, QChar('0')));
}

decltype(Session::annotationTimeMilliseconds) Session::currentTimeSliceMs() const {
    return lastTimeSlice.elapsed();
}

void Session::handleTimeSlice() {
    if (timeSliceActivity) {
        setAnnotationTime(annotationTimeMilliseconds + TIME_SLICE_MS);
        timeSliceActivity = false;
    }
    lastTimeSlice.restart();
}

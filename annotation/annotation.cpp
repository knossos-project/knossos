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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "annotation.h"

#include "dataset.h"
#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"

#include <QApplication>

class Annotation::ActivityEventFilter : public QObject {
    bool & timeSliceActivity;
public:
    ActivityEventFilter(Annotation & annotation) : QObject(&annotation), timeSliceActivity(annotation.timeSliceActivity) {}
    virtual bool eventFilter(QObject *object, QEvent *event) override {
        // mark time slice as valid on any user actions except just moving the mouse
        int type = event->type();
        if (type == QEvent::MouseButtonPress || type == QEvent::KeyPress || type == QEvent::Wheel) {
             timeSliceActivity = true;
        }
        return QObject::eventFilter(object, event);
    }
};

Annotation::Annotation() : annotationMode(AnnotationMode::Mode_Tracing) {
    qApp->installEventFilter(new ActivityEventFilter(*this));

    annotationTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&annotationTimer, &QTimer::timeout, this, &Annotation::handleTimeSlice);
    annotationTimer.start(TIME_SLICE_MS);
    lastTimeSlice.start();

    autoSaveTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&autoSaveTimer, &QTimer::timeout, [this]() {
       if(unsavedChanges) {
           emit autoSaveSignal();
       }
    });
}

bool Annotation::isEmpty() const {
    return annotationFilename.isEmpty() && !Annotation::singleton().unsavedChanges;
}

void Annotation::clearAnnotation() {
    Skeletonizer::singleton().clearSkeleton();
    Segmentation::singleton().clear();
    resetMovementArea();
    setAnnotationTime(0);
    annotationFilename = "";
    extraFiles.clear();
    emit clearedAnnotation();
    unsavedChanges = false;
}

bool Annotation::outsideMovementArea(const Coordinate & pos) {
    const Coordinate magMinPos(pos.x - std::fmod(pos.x, Dataset::current().scaleFactor.x),
                pos.y - std::fmod(pos.y, Dataset::current().scaleFactor.y),
                pos.z - std::fmod(pos.z, Dataset::current().scaleFactor.z));
    const auto maxMagPos = magMinPos + Dataset::current().scaleFactor - 1;
    return maxMagPos.x < movementAreaMin.x || magMinPos.x >= movementAreaMax.x ||
           maxMagPos.y < movementAreaMin.y || magMinPos.y >= movementAreaMax.y ||
           maxMagPos.z < movementAreaMin.z || magMinPos.z >= movementAreaMax.z;
}

void Annotation::updateMovementArea(const Coordinate & min, const Coordinate & max) {
    movementAreaMin = min.capped({0, 0, 0}, Dataset::current().boundary + 1);
    movementAreaMax = max.capped({0, 0, 0}, Dataset::current().boundary + 1);
    emit movementAreaChanged();
}

void Annotation::resetMovementArea() {
    updateMovementArea({0, 0, 0}, Dataset::current().boundary);
}

decltype(Annotation::annotationTimeMilliseconds) Annotation::getAnnotationTime() const {
    return annotationTimeMilliseconds;
}

void Annotation::setAnnotationTime(const decltype(annotationTimeMilliseconds) & ms) {
    annotationTimeMilliseconds = ms;

    const auto absoluteMinutes = annotationTimeMilliseconds / 1000 / 60;
    const auto hours = absoluteMinutes / 60;
    const auto minutes = absoluteMinutes % 60;

    emit annotationTimeChanged(QString("%1:%2 h annotation time").arg(hours).arg(minutes, 2, 10, QChar('0')));
}

decltype(Annotation::annotationTimeMilliseconds) Annotation::currentTimeSliceMs() const {
    return lastTimeSlice.elapsed();
}

void Annotation::handleTimeSlice() {
    if (timeSliceActivity) {
        setAnnotationTime(annotationTimeMilliseconds + TIME_SLICE_MS);
        timeSliceActivity = false;
    }
    lastTimeSlice.restart();
}

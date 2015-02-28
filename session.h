#ifndef SESSION_H
#define SESSION_H

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

#include "coordinate.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

enum AnnotationMode { SkeletonizationMode, SegmentationMode };

class Session : public QObject {
    Q_OBJECT
    class ActivityEventFilter;

    static const int TIME_SLICE_MS = 60 * 1000;

    int annotationTimeMilliseconds = 0;
    bool timeSliceActivity = false;
    QTimer annotationTimer;
    QElapsedTimer lastTimeSlice;

    void handleTimeSlice();

public:
    AnnotationMode annotationMode;

    Coordinate movementAreaMin; // Center of movement area
    Coordinate movementAreaMax; // Range around movement center for every dimension
    bool outsideMovementArea(const Coordinate &pos);
    void updateMovementArea(const Coordinate &min, const Coordinate &max);

    Session();
    static Session & singleton() {
        static Session session;
        return session;
    }
    decltype(annotationTimeMilliseconds) annotationTime() const;
    void annotationTime(const decltype(annotationTimeMilliseconds) & ms);
    decltype(annotationTimeMilliseconds) currentTimeSliceMs() const;

signals:
    void annotationTimeChanged(const QString & timeString);
    void movementAreaChanged();
};

#endif//SESSION_H

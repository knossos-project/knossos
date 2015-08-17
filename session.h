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
#include <QPair>
#include <QString>
#include <QTimer>

enum class AnnotationMode {
    NodeEditing = 0x80,
    LinkedNodes = 0x100,
    UnlinkedNodes = 0x200,
    SkeletonCycles = 0x400,
    Brush = 0x800,
    ObjectSelection = 0x1000,
    ObjectMerge = 0x2000,

    Mode_Tracing = 0x1 | NodeEditing | LinkedNodes,
    Mode_TracingAdvanced = 0x2 | NodeEditing | LinkedNodes | SkeletonCycles,
    Mode_TracingUnlinked = 0x4 | NodeEditing | UnlinkedNodes | SkeletonCycles,

    Mode_Paint = 0x8 | Brush | ObjectSelection,
    Mode_Merge = 0x10 | Brush | ObjectSelection | ObjectMerge,
    Mode_MergeSimple = 0x20 | Brush | ObjectMerge,

    Mode_MergeTracing = 0x40 | NodeEditing | LinkedNodes | SkeletonCycles,
};

class Session : public QObject {
    Q_OBJECT
    class ActivityEventFilter;

    static const int TIME_SLICE_MS = 60 * 1000;

    int annotationTimeMilliseconds = 0;
    bool timeSliceActivity = false;
    QTimer annotationTimer;
    QElapsedTimer lastTimeSlice;

    void handleTimeSlice();

    uint lastSaveTicks = 0;
public:
    bool autoFilenameIncrementBool = true;
    bool autoSaveBool = true;
    uint autoSaveInterval = 5;
    bool unsavedChanges = false;
    void autoSaveIfElapsed();

    QPair<QString, QString> task;
    QFlags<AnnotationMode> annotationMode;

    Coordinate movementAreaMin; // Center of movement area
    Coordinate movementAreaMax; // Range around movement center for every dimension
    bool outsideMovementArea(const Coordinate &pos);
    void updateMovementArea(const Coordinate &min, const Coordinate &max);
    void resetMovementArea();

    QString annotationFilename;

    Session();
    static Session & singleton() {
        static Session session;
        return session;
    }
    decltype(annotationTimeMilliseconds) getAnnotationTime() const;
    void setAnnotationTime(const decltype(annotationTimeMilliseconds) & ms);
    decltype(annotationTimeMilliseconds) currentTimeSliceMs() const;

signals:
    void annotationTimeChanged(const QString & timeString);
    void autosaveSignal();
    void movementAreaChanged();
};

#endif//SESSION_H

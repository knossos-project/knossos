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

#pragma once

#include "coordinate.h"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QPair>
#include <QString>
#include <QTimer>

enum class GUIMode {
    None,
    ProofReading
};

enum AnnotationMode {
    NodeSelection = 1 << 0,
    NodeEditing = 1 << 1 | NodeSelection,
    LinkedNodes = 1 << 2,
    SkeletonCycles = 1 << 3,
    Brush = 1 << 4,
    ObjectSelection = 1 << 5,
    ObjectMerge = 1 << 6,

    Mode_Tracing = (1 << 7) | NodeEditing,
    Mode_TracingAdvanced = (1 << 8) | NodeEditing | SkeletonCycles,

    Mode_Paint = (1 << 9) | Brush | ObjectSelection,
    Mode_OverPaint = (1 << 10) | Brush | ObjectSelection,
    Mode_Merge = (1 << 11) | Brush | ObjectSelection | ObjectMerge,
    Mode_MergeSimple = (1 << 12) | Brush | ObjectMerge,

    Mode_MergeTracing = (1 << 13) | NodeEditing | LinkedNodes | SkeletonCycles,

    Mode_Selection = (1 << 14) | NodeSelection | ObjectSelection,
};

struct AAMTask {
    QString project;
    QString category;
    QString name;

    bool isEmpty() const {
        return project.isEmpty() && category.isEmpty() && name.isEmpty();
    }
    bool operator==(const AAMTask & other) const {
        return project == other.project && category == other.category && name == other.name;
    }
    bool operator!=(const AAMTask & other) const {
        return !(*this == other);
    }
};

class Annotation : public QObject {
    Q_OBJECT
    class ActivityEventFilter;

    static const int TIME_SLICE_MS = 60 * 1000;

    int annotationTimeMilliseconds = 0;
    bool timeSliceActivity = false;
    QTimer annotationTimer;
    QElapsedTimer lastTimeSlice;
    void handleTimeSlice();

public:
    QTimer autoSaveTimer;
    bool autoFilenameIncrementBool = true;
    bool savePlyAsBinary{true};
    bool unsavedChanges = false;

    AAMTask activeTask;
    AAMTask fileTask;
    QFlags<AnnotationMode> annotationMode;
    GUIMode guiMode{GUIMode::None};

    Coordinate movementAreaMin; // Center of movement area
    Coordinate movementAreaMax; // Range around movement center for every dimension
    bool outsideMovementArea(const Coordinate &pos);
    bool outsideMag1MovementArea(const Coordinate & pos);
    void updateMovementArea(const Coordinate &min, const Coordinate &max);
    void resetMovementArea();

    QString annotationFilename;

    QHash<QString, QByteArray> extraFiles;

    Annotation();
    static Annotation & singleton() {
        static Annotation session;
        return session;
    }
    bool isEmpty() const;
    void clearAnnotation();
    decltype(annotationTimeMilliseconds) getAnnotationTime() const;
    void setAnnotationTime(const decltype(annotationTimeMilliseconds) & ms);
    decltype(annotationTimeMilliseconds) currentTimeSliceMs() const;

signals:
    void annotationTimeChanged(const QString & timeString);
    void autoSaveSignal();
    void clearedAnnotation();
    void movementAreaChanged();
};

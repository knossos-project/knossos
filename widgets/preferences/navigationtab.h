/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#ifndef NAVIGATIONTAB_H
#define NAVIGATIONTAB_H

#include <QButtonGroup>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

enum class navigationMode {
    recenter
    , noRecentering
    , additionalVPMove
    , additionalTracingDirectionMove
    , additionalMirroredMove
};

class QSettings;
class NavigationTab : public QWidget {
    Q_OBJECT
    QHBoxLayout mainLayout;
    QVBoxLayout leftLayout;

    QGroupBox movementAreaGroup{"Movement area"};
    QVBoxLayout movementAreaLayout;
    QHBoxLayout areaMinLayout;
    QHBoxLayout areaMaxLayout;
    QSpinBox xMinField, yMinField, zMinField, xMaxField, yMaxField, zMaxField;
    QPushButton resetMovementAreaButton{"Reset to dataset boundary"};

    QGroupBox generalGroup{"General"};
    QFormLayout generalFormLayout;
    QSpinBox movementSpeedSpinBox;
    QSpinBox jumpFramesSpinBox;
    QSpinBox walkFramesSpinBox;

    QGroupBox advancedGroup{"Advanced tracing modes"};
    QVBoxLayout advancedLayout;
    QFormLayout advancedFormLayout;
    QButtonGroup recenteringButtonGroup;
    QRadioButton normalModeButton{"Normal mode (recentering)"};
    QRadioButton noRecenteringButton{"No recentering"};
    QRadioButton additionalViewportDirectionMoveButton{"Additional viewport direction move"};
    QRadioButton additionalTracingDirectionMoveButton{"Additional tracing direction move"};
    QRadioButton additionalMirroredMoveButton{"Additional mirrored move"};
    QSpinBox delayTimePerStepSpinBox;
    QSpinBox numberOfStepsSpinBox;

    void updateMovementArea();
public:
    explicit NavigationTab(QWidget *parent = 0);
    void loadSettings(const QSettings &settings);
    void saveSettings(QSettings &settings);
};

#endif//NAVIGATIONWIDGET_H

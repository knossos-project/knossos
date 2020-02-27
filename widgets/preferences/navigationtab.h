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

#include "widgets/coordinatespins.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

enum class Recentering {
    OnNode
    , Off
    , AheadOfNode
};

class QSettings;
class NavigationTab : public QWidget {
    Q_OBJECT
    QVBoxLayout mainLayout;
    QHBoxLayout upperLayout;
    QVBoxLayout leftupperLayout;

    QGroupBox generalGroup{"General"};
    QFormLayout generalLayout;
    QCheckBox penModeCheckBox{"Pen mode"};
    QCheckBox cubeCoordinateBox{"Show Cube coordinates"};
    QGroupBox movementAreaGroup{"Movement area"};
    QVBoxLayout movementAreaLayout;
    QHBoxLayout minAreaHeadLayout;
    QHBoxLayout minAreaSpinsLayout;
    QHBoxLayout sizeAreaHeadLayout;
    QHBoxLayout sizeAreaSpinsLayout;
    QHBoxLayout maxAreaHeadLayout;
    QHBoxLayout maxAreaSpinsLayout;
    QHBoxLayout movementAreaBottomLayout;
    QLabel minLabel{tr("Min")};
    QPushButton topLeftButton{tr("top left")};
    QLabel sizeLabel{tr("Size")};
    QLabel maxLabel{tr("Max")};
    QPushButton bottomRightButton{tr("bottom right")};
    CoordinateSpins minSpins, sizeSpins, maxSpins;
    QButtonGroup autoGroup;
    QPushButton minAuto{tr("auto-adjust")}, sizeAuto{tr("auto-adjust")}, maxAuto{tr("auto-adjust")};
    QLabel outVisibilityLabel{tr("Visibility of outside area")};
    QHBoxLayout outVisibilityAdjustLayout;
    QSlider outVisibilitySlider;
    QSpinBox outVisibilitySpin;
    QPushButton resetMovementAreaButton{"Reset to dataset boundaries"};
    QFrame separator;
    QGroupBox keyboardMovementGroup{"Keyboard movement"};
    QFormLayout keyboardMovementLayout;
    QSpinBox movementSpeedSpinBox;
    QSpinBox jumpFramesSpinBox;
    QSpinBox walkFramesSpinBox;

    QGroupBox advancedGroup{"Recentering behaviour"};
    QFormLayout advancedFormLayout;
    QButtonGroup recenteringButtonGroup;
    QRadioButton recenteringButton{"Recenter on new node (default)"};
    QRadioButton noRecenteringButton{"No recentering"};
    QRadioButton additionalTracingDirectionMoveButton{"Recenter ahead of tracing direction"};
    QSpinBox delayTimePerStepSpinBox;
    QSpinBox numberOfStepsSpinBox;

    void updateMovementArea();
public:
    explicit NavigationTab(QWidget *parent = nullptr);
    void loadSettings(const QSettings &settings);
    void saveSettings(QSettings &settings);
};

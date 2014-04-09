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

#include "vpskeletonviewportwidget.h"
#include "knossos-global.h"

extern stateInfo * const state;

VPSkeletonViewportWidget::VPSkeletonViewportWidget(QWidget * const parent) : QWidget(parent) {
    line.setFrameShape(QFrame::HLine);
    line.setFrameShadow(QFrame::Sunken);

    line2.setFrameShape(QFrame::HLine);
    line2.setFrameShadow(QFrame::Sunken);

    line3.setFrameShape(QFrame::HLine);
    line3.setFrameShadow(QFrame::Sunken);

    int currentGridColumn = 0;
    gridLayout.addWidget(&skeletonDisplayModesLabel, currentGridColumn++, 0);
    gridLayout.addWidget(&line, currentGridColumn++, 0);
    gridLayout.addWidget(&wholeSkeletonRadioButton, currentGridColumn++, 0);
    gridLayout.addWidget(&onlyActiveTreeRadioButton, currentGridColumn++, 0);
    //gridLayout.addWidget(&onlyCurrentCubeRadioButton, currentGridColumn++, 0);
    gridLayout.addWidget(&hideSkeletonRadioButton, currentGridColumn++, 0);

    gridLayout.addWidget(&datasetVisualizationLabel, currentGridColumn++, 0);
    gridLayout.addWidget(&line2, currentGridColumn++, 0);
    gridLayout.addWidget(&showXYPlaneCheckBox, currentGridColumn++, 0);
    gridLayout.addWidget(&showXZPlaneCheckBox, currentGridColumn++, 0);
    gridLayout.addWidget(&showYZPlaneCheckBox, currentGridColumn++, 0);

    gridLayout.addWidget(&view3dlabel, currentGridColumn++, 0);
    gridLayout.addWidget(&line3, currentGridColumn++, 0);
    gridLayout.addWidget(&rotateAroundActiveNodeCheckBox, currentGridColumn++, 0);

    mainLayout.addLayout(&gridLayout);
    mainLayout.addStretch(1);//expand vertically with empty space
    setLayout(&mainLayout);

    QObject::connect(&wholeSkeletonRadioButton, &QRadioButton::clicked, this, &VPSkeletonViewportWidget::wholeSkeletonSelected);
    //QObject::connect(&onlyCurrentCubeRadioButton, &QRadioButton::clicked, this, &VPSkeletonViewportWidget::onlyCurrentCubeSelected);
    QObject::connect(&onlyActiveTreeRadioButton, &QRadioButton::clicked, this, &VPSkeletonViewportWidget::onlyActiveTreeSelected);
    QObject::connect(&hideSkeletonRadioButton, &QRadioButton::clicked, this, &VPSkeletonViewportWidget::hideSkeletonSelected);
    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showXYplane = checked; });
    QObject::connect(&showYZPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showYZplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->showXZplane = checked; });
    QObject::connect(&rotateAroundActiveNodeCheckBox, &QCheckBox::clicked, [](bool checked){state->skeletonState->rotateAroundActiveNode = checked; });
}

void VPSkeletonViewportWidget::wholeSkeletonSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
    } else {
        state->skeletonState->displayMode &= ~DSP_SKEL_VP_WHOLE;
    }
    emit updateViewerStateSignal();
}

void VPSkeletonViewportWidget::onlyCurrentCubeSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_CURRENTCUBE;
    } else {
        state->skeletonState->displayMode &= ~DSP_SKEL_VP_CURRENTCUBE;
    }
    emit updateViewerStateSignal();
}

void VPSkeletonViewportWidget::onlyActiveTreeSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_ACTIVETREE;
    } else {
        state->skeletonState->displayMode &= ~DSP_ACTIVETREE;
    }
    emit updateViewerStateSignal();
}

void VPSkeletonViewportWidget::hideSkeletonSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
    } else {
        state->skeletonState->displayMode &= ~DSP_SKEL_VP_HIDE;
    }
    emit updateViewerStateSignal();
}

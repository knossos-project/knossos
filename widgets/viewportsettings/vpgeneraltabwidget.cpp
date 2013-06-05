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

#include "vpgeneraltabwidget.h"
#include <QLabel>
#include <QSpinBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include "knossos-global.h"
extern struct stateInfo *state;

VPGeneralTabWidget::VPGeneralTabWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    this->skeletonVisualizationLabel = new QLabel("Skeleton Visualization");
    this->skeletonRenderingModelLabel = new QLabel("Skeleton Rendering Model");

    this->lightEffectsButton = new QCheckBox("Light Effects");
    this->lightEffectsButton->setChecked(state->viewerState->lightOnOff);

    this->hightlightActiveTreeButton = new QCheckBox("Hilight Active Tree");
    this->hightlightActiveTreeButton->setChecked(state->viewerState->highlightVp);

    this->showAllNodeIdsButton = new QCheckBox("Show All Node IDs");
    this->showAllNodeIdsButton->setChecked(state->viewerState->showVPLabels);

    this->overrideNodeRadiusButton = new QCheckBox("Override Node Radius");
    this->overrideNodeRadiusButton->setChecked(state->skeletonState->overrideNodeRadiusBool);

    this->edgeNodeRadiusRatioSpinBox = new QDoubleSpinBox();
    this->edgeNodeRadiusRatioSpinBox->setSingleStep(0.1);
    this->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);
    this->edgeNodeRadiusRatioLabel = new QLabel("Edge <-> Node Radius Ratio:");

    this->linesAndPointsButton = new QRadioButton("Lines and Points(fast)");

    this->skeleton3dButton = new QRadioButton("3D Skeleton");

    if(!state->skeletonState->overrideNodeRadiusBool) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(false);
    }

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(skeletonVisualizationLabel, 1, 1);
    gridLayout->addWidget(skeletonRenderingModelLabel, 1, 2);
    gridLayout->addWidget(line, 2, 1);
    gridLayout->addWidget(line2, 2, 2);

    gridLayout->addWidget(this->lightEffectsButton, 3, 1);
    gridLayout->addWidget(linesAndPointsButton, 3, 2);
    gridLayout->addWidget(hightlightActiveTreeButton, 4, 1);
    gridLayout->addWidget(skeleton3dButton, 4, 2);

    gridLayout->addWidget(showAllNodeIdsButton, 5, 1);
    gridLayout->addWidget(overrideNodeRadiusButton, 6, 1);
    gridLayout->addWidget(edgeNodeRadiusRatioLabel, 7, 1);
    gridLayout->addWidget(edgeNodeRadiusRatioSpinBox, 7, 2);


    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
    mainLayout->addStretch(50);

    connect(lightEffectsButton, SIGNAL(clicked(bool)), this, SLOT(lightEffectsChecked(bool)));
    connect(showAllNodeIdsButton, SIGNAL(clicked(bool)), this, SLOT(showAllNodeIdsChecked(bool)));
    connect(hightlightActiveTreeButton, SIGNAL(clicked(bool)), this, SLOT(hightlightActiveTreeChecked(bool)));
    connect(overrideNodeRadiusButton, SIGNAL(clicked(bool)), this, SLOT(overrideNodeRadiusChecked(bool)));
    connect(edgeNodeRadiusRatioSpinBox, SIGNAL(valueChanged(double)), this, SLOT(edgeNodeRadiusRatioChanged(double)));
    connect(linesAndPointsButton, SIGNAL(clicked(bool)), this, SLOT(linesAndPointsChecked(bool)));
    connect(skeleton3dButton, SIGNAL(clicked(bool)), this, SLOT(skeleton3dChecked(bool)));
}

void VPGeneralTabWidget::lightEffectsChecked(bool on) {
    state->viewerState->lightOnOff = on;
}


void VPGeneralTabWidget::hightlightActiveTreeChecked(bool on) {
    state->skeletonState->highlightActiveTree = on;

    state->skeletonState->skeletonChanged = true;
}


void VPGeneralTabWidget::showAllNodeIdsChecked(bool on) {
    state->skeletonState->showNodeIDs = on;

    state->skeletonState->skeletonChanged = true;
}

void VPGeneralTabWidget::overrideNodeRadiusChecked(bool on) {
    state->skeletonState->overrideNodeRadiusBool = on;

    if(on) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(true);
    } else {
        this->edgeNodeRadiusRatioLabel->setEnabled(false);
    }
}


void VPGeneralTabWidget::edgeNodeRadiusRatioChanged(double value) {
    state->skeletonState->segRadiusToNodeRadius = value;
}


void VPGeneralTabWidget::linesAndPointsChecked(bool on) {
    if(on) {
        state->viewerState->gui->radioRenderingModel = DSP_LINES_POINTS;
    }
}

/**
  * @todo what is the constant for 3D Skeleton ?
  */
void VPGeneralTabWidget::skeleton3dChecked(bool on) {
    if(on) {
        state->viewerState->gui->radioRenderingModel;
    }
}

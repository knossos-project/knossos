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
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QPushButton>
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

    this->lightEffectsCheckBox = new QCheckBox("Light Effects");
    this->lightEffectsCheckBox->setChecked(state->viewerState->lightOnOff);

    this->hightlightActiveTreeCheckBox = new QCheckBox("Hilight Active Tree");
    this->hightlightActiveTreeCheckBox->setChecked(state->viewerState->highlightVp);

    this->showAllNodeIdsCheckBox = new QCheckBox("Show All Node IDs");
    this->showAllNodeIdsCheckBox->setChecked(state->viewerState->showVPLabels);

    this->renderingQualityLabel = new QLabel("Rendering quality(1 best, 20 fastest):");
    this->renderingQualitySpinBox = new QSpinBox();
    this->renderingQualitySpinBox->setMinimum(1);
    this->renderingQualitySpinBox->setMaximum(20);

    this->overrideNodeRadiusCheckBox = new QCheckBox("Override Node Radius");
    this->overrideNodeRadiusCheckBox->setChecked(state->skeletonState->overrideNodeRadiusBool);
    this->overrideNodeRadiusSpinBox = new QDoubleSpinBox();

    this->edgeNodeRadiusRatioSpinBox = new QDoubleSpinBox();
    this->edgeNodeRadiusRatioSpinBox->setSingleStep(0.1);
    this->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);
    this->edgeNodeRadiusRatioLabel = new QLabel("Edge <-> Node Radius Ratio:");

    if(!state->skeletonState->overrideNodeRadiusBool) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(false);
    }

    this->showPosAndResizeCheckBox = new QCheckBox("Show Position and Resize Button");
    this->useStandardPosAndSizeButton = new QPushButton("Use Standard Positions And Sizes");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);


    mainLayout->addWidget(this->skeletonVisualizationLabel);
    mainLayout->addWidget(line);
    mainLayout->addWidget(this->lightEffectsCheckBox);
    mainLayout->addWidget(this->hightlightActiveTreeCheckBox);
    mainLayout->addWidget(this->showAllNodeIdsCheckBox);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(this->renderingQualityLabel);
    hLayout->addWidget(this->renderingQualitySpinBox);
    mainLayout->addLayout(hLayout);

    QHBoxLayout *hLayout2 = new QHBoxLayout();
    hLayout2->addWidget(this->overrideNodeRadiusCheckBox);
    hLayout2->addWidget(this->overrideNodeRadiusSpinBox);
    mainLayout->addLayout(hLayout2);

    QHBoxLayout *hLayout3 = new QHBoxLayout();
    hLayout3->addWidget(this->edgeNodeRadiusRatioLabel);
    hLayout3->addWidget(this->edgeNodeRadiusRatioSpinBox);
    mainLayout->addLayout(hLayout3);

    mainLayout->addWidget(this->showPosAndResizeCheckBox);
    mainLayout->addWidget(this->useStandardPosAndSizeButton);

    setLayout(mainLayout);
    mainLayout->addStretch(50);

    connect(lightEffectsCheckBox, SIGNAL(clicked(bool)), this, SLOT(lightEffectsChecked(bool)));
    connect(showAllNodeIdsCheckBox, SIGNAL(clicked(bool)), this, SLOT(showAllNodeIdsChecked(bool)));
    connect(hightlightActiveTreeCheckBox, SIGNAL(clicked(bool)), this, SLOT(hightlightActiveTreeChecked(bool)));
    connect(renderingQualitySpinBox, SIGNAL(valueChanged(int)), this, SLOT(renderingQualityChanged(int)));
    connect(overrideNodeRadiusCheckBox, SIGNAL(clicked(bool)), this, SLOT(overrideNodeRadiusChecked(bool)));
    connect(edgeNodeRadiusRatioSpinBox, SIGNAL(valueChanged(double)), this, SLOT(edgeNodeRadiusRatioChanged(double)));
    connect(showPosAndResizeCheckBox, SIGNAL(clicked(bool)), this, SLOT(showPosAndSizeChecked(bool)));
    connect(useStandardPosAndSizeButton, SIGNAL(clicked()), this, SLOT(useStandardPosAndSizeClicked()));


}

void VPGeneralTabWidget::lightEffectsChecked(bool on) {
    state->viewerState->lightOnOff = on;
}


void VPGeneralTabWidget::hightlightActiveTreeChecked(bool on) {
    emit highlightActiveTreeSignal(on);
    emit skeletonChangedSignal(true);

    state->skeletonState->highlightActiveTree = on;

    state->skeletonState->skeletonChanged = true;
}


void VPGeneralTabWidget::showAllNodeIdsChecked(bool on) {
    emit showNodeID(on);
    emit skeletonChangedSignal(true);

    state->skeletonState->showNodeIDs = on;

    state->skeletonState->skeletonChanged = true;
}

void VPGeneralTabWidget::overrideNodeRadiusChecked(bool on) {
    emit overrideNodeRadiusSignal(on);
    state->skeletonState->overrideNodeRadiusBool = on;

    if(on) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(true);

    } else {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(false);
    }
}

void VPGeneralTabWidget::overrideNodeRadiusChanged(double value) {
    state->skeletonState->overrideNodeRadiusVal = value;
    state->skeletonState->skeletonChanged = true;
}


void VPGeneralTabWidget::edgeNodeRadiusRatioChanged(double value) {
    emit segRadiusToNodeRadiusSignal(value);
    state->skeletonState->segRadiusToNodeRadius = value;

}

void VPGeneralTabWidget::renderingQualityChanged(int value) {
    state->viewerState->cumDistRenderThres = value;
}

/* @todo */
void VPGeneralTabWidget::showPosAndSizeChecked(bool on) {
    if(on) {
        for(int i = 0; i < state->viewerState->numberViewports; i++) {

        }
    } else {
        for(int i = 0; i < state->viewerState->numberViewports; i++) {

        }
    }


}

/* @tood */
void VPGeneralTabWidget::useStandardPosAndSizeClicked() {

}

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

#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "knossos-global.h"
#include "vpgeneraltabwidget.h"
#include "widgets/mainwindow.h"

extern  stateInfo *state;

VPGeneralTabWidget::VPGeneralTabWidget(QWidget *parent) :
    QWidget(parent)
{
    this->skeletonVisualizationLabel = new QLabel("Skeleton Visualization");

    this->lightEffectsCheckBox = new QCheckBox("Light Effects");
    this->lightEffectsCheckBox->setChecked(state->viewerState->lightOnOff);

    this->hightlightActiveTreeCheckBox = new QCheckBox("Highlight Active Tree");
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

    this->showVPDecorationCheckBox = new QCheckBox("Show Viewport Decorations");
    this->showVPDecorationCheckBox->setChecked(true);
    this->resetVPsButton = new QPushButton("Reset Viewport Positions And Sizes");
    this->resetVPsButton->setFocusPolicy(Qt::NoFocus);

    QVBoxLayout *leftSide = new QVBoxLayout();

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    leftSide->addWidget(this->skeletonVisualizationLabel);
    leftSide->addWidget(line);
    leftSide->addWidget(this->lightEffectsCheckBox);
    leftSide->addWidget(this->hightlightActiveTreeCheckBox);
    leftSide->addWidget(this->showAllNodeIdsCheckBox);
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(this->renderingQualityLabel);
    hLayout->addWidget(this->renderingQualitySpinBox);
    leftSide->addLayout(hLayout);

    QVBoxLayout *rightSide = new QVBoxLayout();
    rightSide->addWidget(new QLabel("Skeleton Display Mode"));
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    rightSide->addWidget(line);
    rightSide->addWidget(&wholeSkeletonRadioButton);
    rightSide->addWidget(&onlySelectedTreesRadioButton);
    rightSide->addWidget(&hideSkeletonOrthoVPsCheckBox);
    rightSide->addWidget(&hideSkeletonSkelVPCheckBox);

    QVBoxLayout *mainLayout = new QVBoxLayout();

    hLayout = new QHBoxLayout();
    hLayout->addLayout(leftSide);
    hLayout->addLayout(rightSide);
    mainLayout->addLayout(hLayout);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    hLayout = new QHBoxLayout();
    hLayout->addWidget(this->overrideNodeRadiusCheckBox);
    hLayout->addWidget(this->overrideNodeRadiusSpinBox);
    mainLayout->addLayout(hLayout);

    hLayout = new QHBoxLayout();
    hLayout->addWidget(this->edgeNodeRadiusRatioLabel);
    hLayout->addWidget(this->edgeNodeRadiusRatioSpinBox);
    mainLayout->addLayout(hLayout);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    this->resetVPsButton->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(this->showVPDecorationCheckBox);
    mainLayout->addWidget(this->resetVPsButton);


    setLayout(mainLayout);
    mainLayout->addStretch(50);

    connect(lightEffectsCheckBox, SIGNAL(clicked(bool)), this, SLOT(lightEffectsChecked(bool)));
    connect(showAllNodeIdsCheckBox, SIGNAL(clicked(bool)), this, SLOT(showAllNodeIdsChecked(bool)));
    connect(hightlightActiveTreeCheckBox, SIGNAL(clicked(bool)), this, SLOT(hightlightActiveTreeChecked(bool)));
    connect(renderingQualitySpinBox, SIGNAL(valueChanged(int)), this, SLOT(renderingQualityChanged(int)));
    connect(overrideNodeRadiusCheckBox, SIGNAL(clicked(bool)), this, SLOT(overrideNodeRadiusChecked(bool)));
    connect(overrideNodeRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(overrideNodeRadiusChanged(double)));

    connect(edgeNodeRadiusRatioSpinBox, SIGNAL(valueChanged(double)), this, SLOT(edgeNodeRadiusRatioChanged(double)));

    QObject::connect(&wholeSkeletonRadioButton, &QRadioButton::clicked, this, &VPGeneralTabWidget::wholeSkeletonSelected);
    //QObject::connect(&onlyCurrentCubeRadioButton, &QRadioButton::clicked, this, &VPGeneralTabWidget::onlyCurrentCubeSelected);
    QObject::connect(&onlySelectedTreesRadioButton, &QRadioButton::clicked, this, &VPGeneralTabWidget::onlySelectedTreesSelected);
    QObject::connect(&hideSkeletonOrthoVPsCheckBox, &QCheckBox::clicked, this, &VPGeneralTabWidget::hideSkeletonOrthoVPClicked);
    QObject::connect(&hideSkeletonSkelVPCheckBox, &QCheckBox::clicked, this, &VPGeneralTabWidget::hideSkeletonSkelVPClicked);

    QObject::connect(showVPDecorationCheckBox, &QCheckBox::toggled, this, &VPGeneralTabWidget::setViewportDecorations);
    QObject::connect(resetVPsButton, &QPushButton::clicked, this, &VPGeneralTabWidget::resetViewportPositions);
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
    overrideNodeRadiusSpinBox->setEnabled(on);
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

void VPGeneralTabWidget::wholeSkeletonSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode &= (~DSP_WHOLE & ~DSP_SELECTED_TREES & ~DSP_CURRENTCUBE);
        state->skeletonState->displayMode |= DSP_WHOLE;
        emit updateViewerStateSignal();
    }
}

void VPGeneralTabWidget::onlyCurrentCubeSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode &= (~DSP_WHOLE & ~DSP_SELECTED_TREES & ~DSP_CURRENTCUBE);
        state->skeletonState->displayMode |= DSP_CURRENTCUBE;
        emit updateViewerStateSignal();
    }
}

void VPGeneralTabWidget::onlySelectedTreesSelected(bool checked) {
    if (checked) {
        state->skeletonState->displayMode &= (~DSP_WHOLE & ~DSP_SELECTED_TREES & ~DSP_CURRENTCUBE);
        state->skeletonState->displayMode |= DSP_SELECTED_TREES;
        emit updateViewerStateSignal();
    }
}

void VPGeneralTabWidget::hideSkeletonSkelVPClicked(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
    }
    else {
        state->skeletonState->displayMode &= ~DSP_SKEL_VP_HIDE;
    }
    emit updateViewerStateSignal();
}

void VPGeneralTabWidget::hideSkeletonOrthoVPClicked(bool checked) {
    if (checked) {
        state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
    } else {
        state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
    }
    emit updateViewerStateSignal();
}

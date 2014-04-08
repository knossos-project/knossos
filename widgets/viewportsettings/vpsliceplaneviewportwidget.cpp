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
#include "vpsliceplaneviewportwidget.h"

#include "widgets/mainwindow.h"

#include <QLabel>
#include <QFrame>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFileDialog>

extern  stateInfo *state;

VPSlicePlaneViewportWidget::VPSlicePlaneViewportWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout = new QGridLayout();

    skeletonOverlayLabel = new QLabel("Skeleton Overlay");
    voxelFilteringLabel = new QLabel("Voxel Filtering");

    enableSkeletonOverlayCheckBox = new QCheckBox("Enable Skeleton Overlay");
    highlightIntersectionsCheckBox = new QCheckBox("Highlight Intersections");
    datasetLinearFilteringCheckBox = new QCheckBox("Dataset Linear Filtering");

    colorLookupTablesLabel = new QLabel("Color Lookup Tables");
    datasetDynamicRangeLabel = new QLabel("Dataset Dynamic Range");

    depthCutoffLabel = new QLabel("Depth Cutoff");
    depthCutoffSpinBox = new QDoubleSpinBox();
    depthCutoffSpinBox->setSingleStep(0.5);
    depthCutoffSpinBox->setMinimum(0.5);

    useOwnDatasetColorsCheckBox = new QCheckBox("Use Own Dataset Colors");
    useOwnTreeColorsCheckBox = new QCheckBox("Use Own Tree Colors");
    useOwnDatasetColorsButton = new QPushButton("Load");
    useOwnTreeColorButton = new QPushButton("Load");

    biasLabel = new QLabel("Bias");
    biasSpinBox = new QSpinBox();
    biasSpinBox->setMaximum(255);
    biasSpinBox->setSingleStep(1);
    biasSlider = new QSlider(Qt::Horizontal);
    biasSlider->setMaximum(255);
    biasSlider->setSingleStep(1);

    rangeDeltaLabel = new QLabel("Range Delta");
    rangeDeltaSlider = new QSlider(Qt::Horizontal);
    rangeDeltaSlider->setMaximum(255);
    rangeDeltaSlider->setSingleStep(1);
    rangeDeltaSpinBox = new QSpinBox();
    rangeDeltaSpinBox->setMaximum(255);
    rangeDeltaSpinBox->setSingleStep(1);

    objectIDOverlayLabel = new QLabel("Object ID Overlay");
    viewportObjectsLabel = new QLabel("Viewport Objects");
    treeLutFile = new QLabel();
    datasetLutFile = new QLabel();

    enableColorOverlayCheckBox = new QCheckBox("Enable Color Overlay");
    drawIntersectionsCrossHairCheckBox = new QCheckBox("Draw Intersections Crosshairs");
    showViewPortsSizeCheckBox = new QCheckBox("Show Viewport Size");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    QFrame *line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);

    QFrame *line5 = new QFrame();
    line5->setFrameShape(QFrame::HLine);
    line5->setFrameShadow(QFrame::Sunken);

    QFrame *line6 = new QFrame();
    line6->setFrameShape(QFrame::HLine);
    line6->setFrameShadow(QFrame::Sunken);

    gridLayout->addWidget(skeletonOverlayLabel, 0, 0);
    gridLayout->addWidget(voxelFilteringLabel, 0, 3);
    gridLayout->addWidget(line, 1, 0, 1, 2);
    gridLayout->addWidget(line2, 1, 3, 1, 3);
    gridLayout->addWidget(enableSkeletonOverlayCheckBox, 2, 0);
    gridLayout->addWidget(datasetLinearFilteringCheckBox, 2, 3);
    gridLayout->addWidget(highlightIntersectionsCheckBox, 3, 0);
    gridLayout->addWidget(depthCutoffLabel, 4, 0);
    gridLayout->addWidget(depthCutoffSpinBox, 4, 1);
    gridLayout->addWidget(colorLookupTablesLabel, 7, 0);
    gridLayout->addWidget(datasetDynamicRangeLabel, 7, 3);
    gridLayout->addWidget(line3, 8, 0, 1, 2);
    gridLayout->addWidget(line4, 8, 3, 1, 3);
    gridLayout->addWidget(useOwnDatasetColorsCheckBox, 9, 0);
    gridLayout->addWidget(useOwnDatasetColorsButton, 9, 1);
    gridLayout->addWidget(biasLabel, 9, 3);
    gridLayout->addWidget(biasSlider, 9, 4);
    gridLayout->addWidget(biasSpinBox, 9, 5);
    gridLayout->addWidget(useOwnTreeColorsCheckBox, 10, 0);
    gridLayout->addWidget(useOwnTreeColorButton, 10, 1);
    gridLayout->addWidget(rangeDeltaLabel, 10, 3);
    gridLayout->addWidget(rangeDeltaSlider, 10, 4);
    gridLayout->addWidget(rangeDeltaSpinBox, 10, 5);
    gridLayout->addWidget(objectIDOverlayLabel, 11, 0);
    gridLayout->addWidget(viewportObjectsLabel, 11, 3);
    gridLayout->addWidget(line5, 12, 0, 1, 2);
    gridLayout->addWidget(line6, 12, 3, 1, 3);
    gridLayout->addWidget(enableColorOverlayCheckBox, 13, 0);
    gridLayout->addWidget(drawIntersectionsCrossHairCheckBox, 13, 3);
    gridLayout->addWidget(showViewPortsSizeCheckBox, 14, 3);

    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);

    QObject::connect(enableSkeletonOverlayCheckBox, &QCheckBox::clicked, this, &VPSlicePlaneViewportWidget::enableSkeletonOverlayClicked);
    connect(datasetLinearFilteringCheckBox, SIGNAL(clicked(bool)), this, SLOT(datasetLinearFilteringChecked(bool)));
    connect(highlightIntersectionsCheckBox, SIGNAL(clicked(bool)), this, SLOT(hightlightIntersectionsChecked(bool)));
    connect(depthCutoffSpinBox, SIGNAL(valueChanged(double)), this, SLOT(depthCutoffChanged(double)));
    QObject::connect(useOwnDatasetColorsCheckBox, &QCheckBox::clicked, this, &VPSlicePlaneViewportWidget::useOwnDatasetColorsClicked);
    connect(useOwnDatasetColorsButton, SIGNAL(clicked()), this, SLOT(useOwnDatasetColorsButtonClicked()));
    QObject::connect(useOwnTreeColorsCheckBox, &QCheckBox::clicked, this, &VPSlicePlaneViewportWidget::useOwnTreeColorsClicked);
    connect(useOwnTreeColorButton, SIGNAL(clicked()), this, SLOT(useOwnTreeColorButtonClicked()));
    connect(biasSlider, SIGNAL(sliderMoved(int)), this, SLOT(biasSliderMoved(int)));
    connect(biasSpinBox, SIGNAL(valueChanged(int)), this, SLOT(biasChanged(int)));
    connect(rangeDeltaSlider, SIGNAL(sliderMoved(int)), this, SLOT(rangeDeltaSliderMoved(int)));
    connect(rangeDeltaSpinBox, SIGNAL(valueChanged(int)), this, SLOT(rangeDeltaChanged(int)));
    connect(enableColorOverlayCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableColorOverlayChecked(bool)));
    connect(drawIntersectionsCrossHairCheckBox, SIGNAL(clicked(bool)), this, SLOT(drawIntersectionsCrossHairChecked(bool)));
    connect(showViewPortsSizeCheckBox, SIGNAL(clicked(bool)), this, SLOT(showViewPortsSizeChecked(bool)));
}

void VPSlicePlaneViewportWidget::enableSkeletonOverlayClicked(bool checked) {
    if (checked) {
        state->skeletonState->displayMode &= ~DSP_SLICE_VP_HIDE;
    } else {
        state->skeletonState->displayMode |= DSP_SLICE_VP_HIDE;
    }
    emit updateViewerStateSignal();
}

void VPSlicePlaneViewportWidget::datasetLinearFilteringChecked(bool checked) {
    if (checked) {
        state->viewerState->filterType = GL_LINEAR;
    } else {
        state->viewerState->filterType = GL_NEAREST;
    }
    emit updateViewerStateSignal();
}

void VPSlicePlaneViewportWidget::hightlightIntersectionsChecked(bool checked) {
    emit showIntersectionsSignal(checked);
    state->skeletonState->showIntersections = checked;
}

void VPSlicePlaneViewportWidget::depthCutoffChanged(double value) {
    state->viewerState->depthCutOff = value;
}

void VPSlicePlaneViewportWidget::useOwnDatasetColorsClicked(bool checked) {
    if (checked && datasetLutFile->text().isEmpty()) {//load file if none is cached
        useOwnDatasetColorsButtonClicked();
    }
    if (!datasetLutFile->text().isEmpty()) {//valid filename → apply
        state->viewerState->datasetColortableOn = checked;
        MainWindow::datasetColorAdjustmentsChanged();
    }
}

void VPSlicePlaneViewportWidget::useOwnDatasetColorsButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Dataset Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut)"));

    if (!fileName.isEmpty()) {//load lut and apply
        datasetLutFile->setText(fileName);
        loadDatasetLUT();
        useOwnDatasetColorsCheckBox->setChecked(true);
        useOwnDatasetColorsCheckBox->clicked(true);
    } else {
        useOwnDatasetColorsCheckBox->setChecked(false);
        useOwnDatasetColorsCheckBox->clicked(false);
    }
}

void VPSlicePlaneViewportWidget::loadDatasetLUT() {

    bool result = loadDataSetColortableSignal(this->datasetLutFile->text(), &(state->viewerState->datasetColortable[0][0]), GL_RGB);

    if(!result) {
        LOG("Error loading Dataset LUT.\n")
        memcpy(&(state->viewerState->datasetColortable[0][0]),
                       &(state->viewerState->neutralDatasetTable[0][0]),
                       RGB_LUTSIZE);
    }

}

void VPSlicePlaneViewportWidget::useOwnTreeColorsClicked(bool checked) {
    if (checked && treeLutFile->text().isEmpty()) {//load file if none is cached
        useOwnTreeColorButtonClicked();
    }
    if (!treeLutFile->text().isEmpty()) {//valid filename → apply
        state->viewerState->treeColortableOn = checked;
        emit treeColorAdjustmentsChangedSignal();
    }
}

void VPSlicePlaneViewportWidget::useOwnTreeColorButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Load Tree Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut)"));

    if(!fileName.isEmpty()) {
        treeLutFile->setText(fileName);
        state->viewerState->treeLutSet = true;//necessary?
        loadTreeLUT();
        useOwnTreeColorsCheckBox->setChecked(true);
        useOwnTreeColorsCheckBox->clicked(true);
    } else {
        useOwnTreeColorsCheckBox->setChecked(false);
        useOwnTreeColorsCheckBox->clicked(false);
    }
}

void VPSlicePlaneViewportWidget::loadTreeLUT() {
    if(loadTreeColorTableSignal(this->treeLutFile->text(), &(state->viewerState->treeColortable[0]), GL_RGB) != true) {
        LOG("Error loading Tree LUT.\n")
        memcpy(&(state->viewerState->treeColortable[0]),
               &(state->viewerState->defaultTreeTable[0]),
                 RGB_LUTSIZE);
        state->viewerState->treeLutSet = false;
    }
}

void VPSlicePlaneViewportWidget::biasSliderMoved(int value) {
    state->viewerState->luminanceBias = value;
    biasSpinBox->setValue(value);
    MainWindow::datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::biasChanged(int value) {
    state->viewerState->luminanceBias = value;
    biasSlider->setValue(value);
    MainWindow::datasetColorAdjustmentsChanged();
}


void VPSlicePlaneViewportWidget::rangeDeltaSliderMoved(int value) {
    state->viewerState->luminanceRangeDelta = value;
    rangeDeltaSpinBox->setValue(value);
    MainWindow::datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::rangeDeltaChanged(int value) {
    state->viewerState->luminanceRangeDelta = value;
    rangeDeltaSlider->setValue(value);
    MainWindow::datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::enableColorOverlayChecked(bool on) {
    state->viewerState->overlayVisible = on;
}

void VPSlicePlaneViewportWidget::drawIntersectionsCrossHairChecked(bool on) {
    state->viewerState->drawVPCrosshairs = on;
}


void VPSlicePlaneViewportWidget::showViewPortsSizeChecked(bool on) {
    state->viewerState->showVPLabels = on;
}

void VPSlicePlaneViewportWidget::updateIntersection() {
    this->drawIntersectionsCrossHairCheckBox->setChecked(state->viewerState->drawVPCrosshairs);
}

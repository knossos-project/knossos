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

#include "segmentation/segmentation.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"
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

VPSlicePlaneViewportWidget::VPSlicePlaneViewportWidget(QWidget *parent) : QWidget(parent), lutErrorBox(this) {
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout = new QGridLayout();

    skeletonOverlayLabel = new QLabel("Skeleton Overlay");
    voxelFilteringLabel = new QLabel("Voxel Filtering");

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

    viewportObjectsLabel = new QLabel("Viewport Objects");
    treeLutFile = new QLabel();
    datasetLutFile = new QLabel();

    segmenationOverlaySpinBox.setMaximum(255);
    segmenationOverlaySpinBox.setSingleStep(1);
    segmenationOverlaySlider.setMaximum(255);
    segmenationOverlaySlider.setSingleStep(1);
    segmentationOverlayLayout.addWidget(&segmenationOverlayLabel);
    segmentationOverlayLayout.addWidget(&segmenationOverlaySlider);
    segmentationOverlayLayout.addWidget(&segmenationOverlaySpinBox);

    drawIntersectionsCrossHairCheckBox = new QCheckBox("Draw Intersections Crosshairs");

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

    gridLayout->addWidget(&arbitraryModeCheckBox, 0, 0);
    gridLayout->addWidget(skeletonOverlayLabel, 1, 0);
    gridLayout->addWidget(voxelFilteringLabel, 1, 3, 1, 3);
    gridLayout->addWidget(line, 2, 0, 1, 2);
    gridLayout->addWidget(line2, 2, 3, 1, 3);
    gridLayout->addWidget(datasetLinearFilteringCheckBox, 3, 3, 1, 3);
    gridLayout->addWidget(highlightIntersectionsCheckBox, 3, 0);
    gridLayout->addWidget(depthCutoffLabel, 5, 0);
    gridLayout->addWidget(depthCutoffSpinBox, 5, 1);
    gridLayout->addWidget(&showNodeCommentsCheckBox, 6, 0);
    gridLayout->addWidget(colorLookupTablesLabel, 8, 0);
    gridLayout->addWidget(datasetDynamicRangeLabel, 8, 3, 1, 3);
    gridLayout->addWidget(line3, 9, 0, 1, 2);
    gridLayout->addWidget(line4, 9, 3, 1, 3);
    gridLayout->addWidget(useOwnDatasetColorsCheckBox, 10, 0);
    gridLayout->addWidget(useOwnDatasetColorsButton, 10, 1);
    gridLayout->addWidget(biasLabel, 10, 3);
    gridLayout->addWidget(biasSlider, 10, 4);
    gridLayout->addWidget(biasSpinBox, 10, 5);
    gridLayout->addWidget(useOwnTreeColorsCheckBox, 11, 0);
    gridLayout->addWidget(useOwnTreeColorButton, 11, 1);
    gridLayout->addWidget(rangeDeltaLabel, 11, 3);
    gridLayout->addWidget(rangeDeltaSlider, 11, 4);
    gridLayout->addWidget(rangeDeltaSpinBox, 11, 5);
    gridLayout->addWidget(&segmenationOverlayGroupLabel, 12, 0);
    gridLayout->addWidget(viewportObjectsLabel, 12, 3);
    gridLayout->addWidget(line5, 13, 0, 1, 2);
    gridLayout->addWidget(line6, 13, 3, 1, 3);
    gridLayout->addLayout(&segmentationOverlayLayout, 14, 0, 1, 2);
    gridLayout->addWidget(drawIntersectionsCrossHairCheckBox, 14, 3, 1, 2);

    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);

    lutErrorBox.setIcon(QMessageBox::Warning);
    lutErrorBox.setText("LUT loading failed");
    lutErrorBox.setInformativeText("LUTs are restricted to 256 RGB tuples");

    QObject::connect(&arbitraryModeCheckBox, &QCheckBox::clicked, [&](bool checked) {
        Viewport::arbitraryOrientation = checked;
        emit setVPOrientationSignal(checked);
    });
    connect(datasetLinearFilteringCheckBox, SIGNAL(clicked(bool)), this, SLOT(datasetLinearFilteringChecked(bool)));
    connect(highlightIntersectionsCheckBox, SIGNAL(clicked(bool)), this, SLOT(hightlightIntersectionsChecked(bool)));
    connect(depthCutoffSpinBox, SIGNAL(valueChanged(double)), this, SLOT(depthCutoffChanged(double)));
    QObject::connect(&showNodeCommentsCheckBox, &QCheckBox::clicked, [&](bool checked){
        Viewport::showNodeComments = checked;
    });
    QObject::connect(useOwnDatasetColorsCheckBox, &QCheckBox::clicked, this, &VPSlicePlaneViewportWidget::useOwnDatasetColorsClicked);
    QObject::connect(useOwnDatasetColorsButton, &QPushButton::clicked, [this](){ useOwnDatasetColorsButtonClicked(); });
    QObject::connect(useOwnTreeColorsCheckBox, &QCheckBox::clicked, this, &VPSlicePlaneViewportWidget::useOwnTreeColorsClicked);
    QObject::connect(useOwnTreeColorButton, &QPushButton::clicked, [this](){ useOwnTreeColorButtonClicked(); });
    QObject::connect(biasSlider, &QSlider::valueChanged, this, &VPSlicePlaneViewportWidget::biasSliderMoved);
    QObject::connect(biasSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VPSlicePlaneViewportWidget::biasChanged);
    QObject::connect(rangeDeltaSlider, &QSlider::valueChanged, this, &VPSlicePlaneViewportWidget::rangeDeltaSliderMoved);
    QObject::connect(rangeDeltaSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VPSlicePlaneViewportWidget::rangeDeltaChanged);
    QObject::connect(&segmenationOverlaySlider, &QSlider::valueChanged, [&](int value){
        segmenationOverlaySpinBox.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    QObject::connect(&segmenationOverlaySpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
        segmenationOverlaySlider.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    connect(drawIntersectionsCrossHairCheckBox, SIGNAL(clicked(bool)), this, SLOT(drawIntersectionsCrossHairChecked(bool)));
}

void VPSlicePlaneViewportWidget::datasetLinearFilteringChecked(bool checked) {
    if (checked) {
        state->viewer->applyTextureFilterSetting(GL_LINEAR);
    } else {
        state->viewer->applyTextureFilterSetting(GL_NEAREST);
    }
}

void VPSlicePlaneViewportWidget::hightlightIntersectionsChecked(bool checked) {
    emit showIntersectionsSignal(checked);
    state->skeletonState->showIntersections = checked;
}

void VPSlicePlaneViewportWidget::depthCutoffChanged(double value) {
    state->viewerState->depthCutOff = value;
}

void VPSlicePlaneViewportWidget::useOwnDatasetColorsClicked(bool checked) {
    if (checked) {//load file if none is cached
        useOwnDatasetColorsButtonClicked(datasetLutFile->text());
    } else {
        state->viewer->defaultDatasetLUT();
    }
}

void VPSlicePlaneViewportWidget::useOwnDatasetColorsButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Dataset Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            state->viewer->loadDatasetLUT(path);
            datasetLutFile->setText(path);
            useOwnDatasetColorsCheckBox->setChecked(true);
        } catch (...) {
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            useOwnDatasetColorsCheckBox->setChecked(false);
        }
    } else {
        useOwnDatasetColorsCheckBox->setChecked(false);
    }
}

void VPSlicePlaneViewportWidget::useOwnTreeColorsClicked(bool checked) {
    if (checked) {//load file if none is cached
        useOwnTreeColorButtonClicked(treeLutFile->text());
    } else {
        Skeletonizer::singleton().loadTreeLUT();
    }
}

void VPSlicePlaneViewportWidget::useOwnTreeColorButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Tree Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            Skeletonizer::singleton().loadTreeLUT(path);
            treeLutFile->setText(path);
            useOwnTreeColorsCheckBox->setChecked(true);
        }  catch (...) {
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            useOwnTreeColorsCheckBox->setChecked(false);
        }
    } else {
        useOwnTreeColorsCheckBox->setChecked(false);
    }
}

void VPSlicePlaneViewportWidget::biasSliderMoved(int value) {
    state->viewerState->luminanceBias = value;
    biasSpinBox->setValue(value);
    state->viewer->datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::biasChanged(int value) {
    state->viewerState->luminanceBias = value;
    biasSlider->setValue(value);
    state->viewer->datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::rangeDeltaSliderMoved(int value) {
    state->viewerState->luminanceRangeDelta = value;
    rangeDeltaSpinBox->setValue(value);
    state->viewer->datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::rangeDeltaChanged(int value) {
    state->viewerState->luminanceRangeDelta = value;
    rangeDeltaSlider->setValue(value);
    state->viewer->datasetColorAdjustmentsChanged();
}

void VPSlicePlaneViewportWidget::drawIntersectionsCrossHairChecked(bool on) {
    state->viewerState->drawVPCrosshairs = on;
}

void VPSlicePlaneViewportWidget::updateIntersection() {
    drawIntersectionsCrossHairCheckBox->setChecked(state->viewerState->drawVPCrosshairs);
}

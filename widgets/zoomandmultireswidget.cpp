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

#include "zoomandmultireswidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QCheckBox>
#include <QDebug>
#include "knossos-global.h"
#include "viewer.h"
#include "mainwindow.h"

extern struct stateInfo *state;

/**
  * In GUI.c some labels are changed:
  * highest Available Mag = lowest Avaible Mag and otherwise. Why ??
  *
  * In zoomDefaults the vpConfigs are reset to ZOOM_MIN and ZOOM_MAX
  * but if the spinboxes are changed it is state->viewerState->zoomOrthoVps, why ??
  */


ZoomAndMultiresWidget::ZoomAndMultiresWidget(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle("Zoom and Multiresolution Settings");
    // top layout
    QGridLayout *topLayout = new QGridLayout();
    this->orthogonalDataViewportLabel = new QLabel("Orthogonal Data Viewport");
    this->skeletonViewLabel = new QLabel("Skeleton View");

    this->orthogonalDataViewportSlider = new QSlider(Qt::Horizontal);
    this->orthogonalDataViewportSlider->setMaximum(100);
    this->skeletonViewSlider = new QSlider(Qt::Horizontal);
    this->skeletonViewSlider->setMaximum(100);

    this->orthogonalDataViewportSpinBox = new QDoubleSpinBox();
    this->orthogonalDataViewportSpinBox->setMaximum(1.00);
    this->orthogonalDataViewportSpinBox->setMinimum(0.02);
    this->orthogonalDataViewportSpinBox->setSingleStep(0.01);

    this->skeletonViewSpinBox = new QDoubleSpinBox();
    this->skeletonViewSpinBox->setMaximum(0.5);
    this->skeletonViewSpinBox->setMinimum(0.0);
    this->skeletonViewSpinBox->setSingleStep(0.01);

    topLayout->addWidget(this->orthogonalDataViewportLabel, 0, 1);
    topLayout->addWidget(this->orthogonalDataViewportSlider, 0, 2);
    topLayout->addWidget(this->orthogonalDataViewportSpinBox, 0, 3);

    topLayout->addWidget(this->skeletonViewLabel, 1, 1);
    topLayout->addWidget(this->skeletonViewSlider, 1, 2);
    topLayout->addWidget(this->skeletonViewSpinBox, 1, 3);

    this->zoomDefaultsButton = new QPushButton("All Zoom defaults");
    topLayout->addWidget(zoomDefaultsButton, 2, 1);

    // main layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(topLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);
    mainLayout->addSpacing(10);

    this->lockDatasetLabel = new QLabel("Lock dataset to current mag");
    this->lockDatasetCheckBox = new QCheckBox();

    QString currentActiveMag = QString("Currently active mag dataset: %1").arg(state->magnification);
    QString highestActiveMag = QString("Highest active mag dataset: %1").arg(state->highestAvailableMag);
    QString lowestActiveMag = QString("Lowest active mag dataset: %1").arg(state->lowestAvailableMag);

    this->currentActiveMagDatasetLabel = new QLabel(currentActiveMag);
    this->highestActiveMagDatasetLabel = new QLabel(highestActiveMag);
    this->lowestActiveMagDatasetLabel = new QLabel(lowestActiveMag);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(lockDatasetLabel, 0, 1);
    gridLayout->addWidget(lockDatasetCheckBox, 0, 2);

    mainLayout->addLayout(gridLayout);

    mainLayout->addWidget(currentActiveMagDatasetLabel);
    mainLayout->addWidget(highestActiveMagDatasetLabel);
    mainLayout->addWidget(lowestActiveMagDatasetLabel);


    this->setLayout(mainLayout);

    connect(this->orthogonalDataViewportSlider, SIGNAL(valueChanged(int)), this, SLOT(orthogonalSliderSlot(int)));
    connect(this->orthogonalDataViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(orthogonalSpinBoxSlot(double)));

    connect(this->skeletonViewSlider, SIGNAL(valueChanged(int)), this, SLOT(skeletonSliderSlot(int)));
    connect(this->skeletonViewSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxSlot(double)));

    connect(this->zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsSlot()));
    connect(this->lockDatasetCheckBox, SIGNAL(toggled(bool)), this, SLOT(lockDatasetMagSlot(bool)));
}

/**
  * As Slider works with integer the range from 0.02 to 1.00 is stored in 100 units
  */
void ZoomAndMultiresWidget::orthogonalSliderSlot(int value) {
    float result = value / 100.0;
    this->orthogonalDataViewportSpinBox->setValue(result);
    state->viewerState->gui->zoomOrthoVPs = result;

}

/**
  * As Slider works with integer the double values from the spinbox are converted with a factor of 100
  */
void ZoomAndMultiresWidget::orthogonalSpinBoxSlot(double value) {
    this->orthogonalDataViewportSlider->setValue(value * 100);
    state->viewerState->gui->zoomOrthoVPs = value;
}

/**
  * Again the slider only works with integer. The range from 0.00 to 0.05 is stored in 200 units
  */
void ZoomAndMultiresWidget::skeletonSliderSlot(int value) {
    float result = value / 200.0;
    this->skeletonViewSpinBox->setValue(result);
    state->skeletonState->zoomLevel = result;

}

/**
  * The double values from the spinbox are converted with a factor of 200
  */
void ZoomAndMultiresWidget::skeletonSpinBoxSlot(double value) {
   this->skeletonViewSlider->setValue(value * 200);

    state->skeletonState->zoomLevel = (float) value;
}

void ZoomAndMultiresWidget::lockDatasetMagSlot(bool on) {
    if(on) {
        state->viewerState->datasetMagLock = true;
    } else {
        state->viewerState->datasetMagLock = false;
    }
}

void ZoomAndMultiresWidget::zoomDefaultsSlot() {
    const int MIN_ZOOM = 0.02;
    state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = MIN_ZOOM;
    state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = MIN_ZOOM;
    state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = MIN_ZOOM;

    orthogonalDataViewportSlider->setValue(MIN_ZOOM);
    orthogonalDataViewportSpinBox->setValue(MIN_ZOOM);
    skeletonViewSlider->setValue(MIN_ZOOM);
    skeletonViewSpinBox->setValue(MIN_ZOOM);

    state->skeletonState->zoomLevel = 0.0;
    Viewer::refreshViewports();

}

void ZoomAndMultiresWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    MainWindow *parent = (MainWindow *) this->parentWidget();
    parent->uncheckZoomAndMultiresAction();
}

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
#include "GUIConstants.h"
#include <QGridLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include "knossos-global.h"

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
    settings = new QSettings();

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

    setLayout(mainLayout);

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
    emit refreshSignal();
}

/**
  * As Slider works with integer the double values from the spinbox are converted with a factor of 100
  */
void ZoomAndMultiresWidget::orthogonalSpinBoxSlot(double value) {
    this->orthogonalDataViewportSlider->setValue(value * 100);
    state->viewerState->gui->zoomOrthoVPs = value;

    emit refreshSignal();
}

/**
  * Again the slider only works with integer. The range from 0.00 to 0.05 is stored in 200 units
  */
void ZoomAndMultiresWidget::skeletonSliderSlot(int value) {
    float result = value / 200.0;
    this->skeletonViewSpinBox->setValue(result);
    state->skeletonState->zoomLevel = result;

    emit refreshSignal();
}

/**
  * The double values from the spinbox are converted with a factor of 200
  */
void ZoomAndMultiresWidget::skeletonSpinBoxSlot(double value) {
   this->skeletonViewSlider->setValue(value * 200);

    state->skeletonState->zoomLevel = (float) value;

    emit refreshSignal();
}

void ZoomAndMultiresWidget::lockDatasetMagSlot(bool on) {
    if(on) {
        state->viewerState->datasetMagLock = true;
    } else {
        state->viewerState->datasetMagLock = false;
    }
}

void ZoomAndMultiresWidget::zoomDefaultsSlot() {
    const float MIN_ZOOM = 0.02;
    state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel = MIN_ZOOM;
    state->viewerState->vpConfigs[VIEWPORT_XZ].texture.zoomLevel = MIN_ZOOM;
    state->viewerState->vpConfigs[VIEWPORT_YZ].texture.zoomLevel = MIN_ZOOM;

    orthogonalDataViewportSlider->setValue(MIN_ZOOM);
    orthogonalDataViewportSpinBox->setValue(MIN_ZOOM);
    skeletonViewSlider->setValue(MIN_ZOOM);
    skeletonViewSpinBox->setValue(MIN_ZOOM);

    state->skeletonState->zoomLevel = 0.0;
    emit refreshSignal();


}

void ZoomAndMultiresWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();

}

void ZoomAndMultiresWidget::update() {
    orthogonalDataViewportSpinBox->setValue(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel);
}

void ZoomAndMultiresWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(ZOOM_AND_MULTIRES_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();
    this->orthogonalDataViewportSpinBox->setValue(settings.value(ORTHO_DATA_VIEWPORTS).toDouble());
    this->skeletonViewSpinBox->setValue(settings.value(SKELETON_VIEW).toDouble());
    this->lockDatasetCheckBox->setChecked(settings.value(LOCK_DATASET_TO_CURRENT_MAG).toBool());
    settings.endGroup();
}

void ZoomAndMultiresWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(ZOOM_AND_MULTIRES_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.setValue(ORTHO_DATA_VIEWPORTS, this->orthogonalDataViewportSpinBox->value());
    settings.setValue(SKELETON_VIEW, this->skeletonViewSpinBox->value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, this->lockDatasetCheckBox->isChecked());
    settings.endGroup();
}

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
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include <QSpacerItem>
#include <QApplication>
#include <QDesktopWidget>
#include "knossos-global.h"

extern  stateInfo *state;

ZoomAndMultiresWidget::ZoomAndMultiresWidget(QWidget *parent) :
    QDialog(parent), lastZoomSkel(0), userZoomSkel(true)
{
    setWindowTitle("Zoom and Multiresolution Settings");

    // top layout
    QGridLayout *topLayout = new QGridLayout();
    this->orthogonalDataViewportLabel = new QLabel(QString("Orthogonal Data Viewport (mag %1)").arg(state->magnification));
    this->skeletonViewportLabel = new QLabel("Skeleton Viewport");

    this->orthogonalDataViewportSpinBox = new QDoubleSpinBox();
    this->orthogonalDataViewportSpinBox->setMaximum(100);
    this->orthogonalDataViewportSpinBox->setMinimum(0);
    this->orthogonalDataViewportSpinBox->setSuffix("\%");

    this->skeletonViewportSpinBox = new QDoubleSpinBox();
    this->skeletonViewportSpinBox->setMaximum(100);
    this->skeletonViewportSpinBox->setMinimum(0);
    this->skeletonViewportSpinBox->setSuffix("\%");

    topLayout->addWidget(this->orthogonalDataViewportLabel, 0, 1);
    topLayout->addWidget(this->orthogonalDataViewportSpinBox, 0, 2);

    topLayout->addWidget(this->skeletonViewportLabel, 1, 1);
    topLayout->addWidget(this->skeletonViewportSpinBox, 1, 2);

    this->zoomDefaultsButton = new QPushButton("All Zoom defaults");
    topLayout->addWidget(zoomDefaultsButton, 2, 1);

    // main layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(topLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);

    this->lockDatasetCheckBox = new QCheckBox("Lock dataset to current mag");

    QString currentActiveMag = QString("Currently active mag dataset: %1").arg(state->magnification);
    QString highestActiveMag = QString("Highest active mag dataset: %1").arg(state->highestAvailableMag);
    QString lowestActiveMag = QString("Lowest active mag dataset: %1").arg(state->lowestAvailableMag);

    this->currentActiveMagDatasetLabel = new QLabel(currentActiveMag);
    this->highestActiveMagDatasetLabel = new QLabel(highestActiveMag);
    this->lowestActiveMagDatasetLabel = new QLabel(lowestActiveMag);

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(lockDatasetCheckBox, 0, 1);

    mainLayout->addLayout(gridLayout);

    mainLayout->addWidget(currentActiveMagDatasetLabel);
    mainLayout->addWidget(highestActiveMagDatasetLabel);
    mainLayout->addWidget(lowestActiveMagDatasetLabel);

    mainLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    setLayout(mainLayout);

    connect(this->orthogonalDataViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(orthogonalSpinBoxChanged(double)));
    connect(this->skeletonViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxChanged(double)));

    connect(this->zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsClicked()));
    connect(this->lockDatasetCheckBox, SIGNAL(toggled(bool)), this, SLOT(lockDatasetMagChecked(bool)));
}

/**
 * @brief ZoomAndMultiresWidget::orthogonalSpinBoxChanged  zooms in and out of ortho viewports with constant step.
 * @param value increment/decrement zoom by value
 */
void ZoomAndMultiresWidget::orthogonalSpinBoxChanged(double value) {
    for(int i = 0; i < VIEWPORT_SKELETON; i++) {
        // conversion from percent to zoom level is inverted, because VPZOOMMAX < VPZOOMMIN,
        // because zoom level directly translates to displayed edge length
        float zoomLevel = 1 - value/100*VPZOOMMIN;
        if(zoomLevel < VPZOOMMAX) {
            zoomLevel = VPZOOMMAX;
        }
        state->viewerState->vpConfigs[i].texture.zoomLevel = zoomLevel;
    }
}
/**
 * @brief ZoomAndMultiresWidget::skeletonSpinBoxChanged increments or decrements the zoom. The step width is relative to
 *        the zoom value. A very high zoom level has smaller zoom steps and vice versa to give a smooth impression to the user.
 * @param value the new value. Used to detect if a zoom in or zoom out is requested.
 *
 * 'lastZoomSkel' stores the last spin value and helps detecting if user wants to zoom in or zoom out, since Qt has no way to
 * know if a value change was an increment or a decrement. Based on that a zoom in or zoom out is performed and the
 * spin box will be updated to the resulting zoom level in percent.
 *
 * But Qt also does not distinguish between value change via the GUI or programmatically. In both cases a signal is emitted,
 * so that the slot is called repeatedly. 'userZoomSkel' tells us if the value change was done via the GUI
 * or programmatically. A programmatic change of the spin box value should not lead to another zoom.
 */
void ZoomAndMultiresWidget::skeletonSpinBoxChanged(double value) {
    if(userZoomSkel) {
        userZoomSkel = false;
        if((value > lastZoomSkel and value < lastZoomSkel + 2
           or value < lastZoomSkel and value > lastZoomSkel - 2) == false) {
            // difference at least greater than two,
            // so user entered a number instead of clicking the up and down buttons
            state->skeletonState->zoomLevel = value/100*SKELZOOMMAX;
            state->skeletonState->viewChanged = true;
        }
        else { // up or down button pressed, find out which.
            if(value > lastZoomSkel and value < lastZoomSkel + 2) { // user wants to zoom in
                emit zoomInSkeletonVPSignal();
            }
            else if(value < lastZoomSkel and value > lastZoomSkel - 2) { // user wants to zoom out
                emit zoomOutSkeletonVPSignal();
            }
            // the following line will lead to signal emission and another call to this slot,
            // but since userZoomSkel was set to false above, no further recursion takes place.
            skeletonViewportSpinBox->setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);
        }
        lastZoomSkel = skeletonViewportSpinBox->value();
        userZoomSkel = true;
    }
    emit zoomLevelSignal(value);
}

void ZoomAndMultiresWidget::lockDatasetMagChecked(bool on) {
    lockDatasetCheckBox->setChecked(on); // neccessary, because this slot is not only connected to the checkbox.
    if(on) {
        state->viewerState->datasetMagLock = true;
    } else {
        state->viewerState->datasetMagLock = false;
    }
}

/**
 * @brief ZoomAndMultiresWidget::zoomDefaultsClicked restores lowest zoom level on all viewports
 */
void ZoomAndMultiresWidget::zoomDefaultsClicked() {
    orthogonalDataViewportSpinBox->setValue(0);

    userZoomSkel = false;
    skeletonViewportSpinBox->setValue(100*SKELZOOMMIN/SKELZOOMMAX);
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    userZoomSkel = true;

    emit zoomLevelSignal(0.0);
}

void ZoomAndMultiresWidget::closeEvent(QCloseEvent *event) {
    this->hide();   
}

void ZoomAndMultiresWidget::update() {
    orthogonalDataViewportLabel->setText(QString("Orthogonal Data Viewport (mag %1)").arg(state->magnification));
    float vpZoomPercent;
    if(state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel == (float)VPZOOMMAX) {
        vpZoomPercent = 100;
    }
    else {
        vpZoomPercent = (1 - state->viewerState->vpConfigs[VIEWPORT_XY].texture.zoomLevel)*100;
    }
    orthogonalDataViewportSpinBox->setValue(vpZoomPercent);
    skeletonViewportSpinBox->setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);

    QString currentActiveMag = QString("Currently active mag dataset: %1").arg(state->magnification);
    QString highestActiveMag = QString("Highest active mag dataset: %1").arg(state->highestAvailableMag);
    QString lowestActiveMag = QString("Lowest active mag dataset: %1").arg(state->lowestAvailableMag);

    this->currentActiveMagDatasetLabel->setText(currentActiveMag);
    this->highestActiveMagDatasetLabel->setText(highestActiveMag);
    this->lowestActiveMagDatasetLabel->setText(lowestActiveMag);
}

void ZoomAndMultiresWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(ZOOM_AND_MULTIRES_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().center().x();
        y = QApplication::desktop()->screen()->rect().center().y();
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();


    if(settings.value(ORTHO_DATA_VIEWPORTS).isNull() == false) {
        this->orthogonalDataViewportSpinBox->setValue(settings.value(ORTHO_DATA_VIEWPORTS).toDouble());
    } else {
        this->orthogonalDataViewportSpinBox->setValue(0);
    }

    if(settings.value(SKELETON_VIEW).isNull() == false) {
        userZoomSkel = false;
        this->skeletonViewportSpinBox->setValue(settings.value(SKELETON_VIEW).toDouble());
        userZoomSkel = true;
        lastZoomSkel = this->skeletonViewportSpinBox->value();
        state->skeletonState->zoomLevel = 0.01*lastZoomSkel * SKELZOOMMAX;
    } else {
        this->skeletonViewportSpinBox->setValue(0);
    }

    state->viewerState->datasetMagLock =
            (settings.value(LOCK_DATASET_TO_CURRENT_MAG).isNull())? true : settings.value(LOCK_DATASET_TO_CURRENT_MAG).toBool();
    this->lockDatasetCheckBox->setChecked(state->viewerState->datasetMagLock);

    settings.endGroup();
    if(visible) {
        show();
    }
    else {
        hide();
    }
    setGeometry(x, y, width, height);
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
    settings.setValue(SKELETON_VIEW, this->skeletonViewportSpinBox->value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, this->lockDatasetCheckBox->isChecked());
    settings.endGroup();
}



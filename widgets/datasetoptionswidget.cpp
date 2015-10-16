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
#include "datasetoptionswidget.h"

#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/viewport.h"

#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QFrame>
#include <QCheckBox>
#include <QDebug>
#include <QSettings>
#include <QSpacerItem>
#include <QApplication>
#include <QDesktopWidget>

DatasetOptionsWidget::DatasetOptionsWidget(QWidget *parent) :
    QDialog(parent), lastZoomSkel(0), userZoomSkel(true) {
    setWindowIcon(QIcon(":/resources/icons/zoom-in.png"));
    setWindowTitle("Dataset Options");

    // compression section
    switch(state->compressionRatio) {
    case 0:
        compressionLabel.setText("Current compression: none (toggle with (5))");
        break;
    case 1000:
        compressionLabel.setText("Current compression: jpg (toggle with (5))");
        break;
    case 1001:
        compressionLabel.setText("Current compression: j2k (toggle with (5))");
        break;
    default:
        compressionLabel.setText("Current compression: jp2 (toggle with (5))");
    }

    // zoom section
    orthogonalDataViewportLabel = new QLabel(QString("Orthogonal Data Viewport (mag %1)").arg(state->magnification));
    skeletonViewportLabel = new QLabel("Skeleton Viewport");
    orthogonalDataViewportSpinBox = new QDoubleSpinBox();
    orthogonalDataViewportSpinBox->setMaximum(100);
    orthogonalDataViewportSpinBox->setMinimum(0);
    orthogonalDataViewportSpinBox->setSuffix(" %");
    skeletonViewportSpinBox = new QDoubleSpinBox();
    skeletonViewportSpinBox->setMaximum(100);
    skeletonViewportSpinBox->setMinimum(0);
    skeletonViewportSpinBox->setSuffix(" %");
    zoomDefaultsButton = new QPushButton("All Zoom defaults");
    zoomDefaultsButton->setAutoDefault(false);

    // multires section
    lockDatasetCheckBox = new QCheckBox("Lock dataset to current mag");
    currentActiveMagDatasetLabel = new QLabel(QString("Currently active mag dataset: %1").arg(state->magnification));
    highestActiveMagDatasetLabel = new QLabel(QString("Highest available mag dataset: %1").arg(state->highestAvailableMag));
    lowestActiveMagDatasetLabel = new QLabel(QString("Lowest available mag dataset: %1").arg(state->lowestAvailableMag));

    QGridLayout *zoomLayout = new QGridLayout();
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    zoomLayout->addWidget(&zoomSectionLabel, 0, 0);
    zoomLayout->addWidget(line, 1, 0, 1, 2);
    zoomLayout->addWidget(orthogonalDataViewportLabel, 2, 0);
    zoomLayout->addWidget(orthogonalDataViewportSpinBox, 2, 1);
    zoomLayout->addWidget(skeletonViewportLabel, 3, 0);
    zoomLayout->addWidget(skeletonViewportSpinBox, 3, 1);
    zoomLayout->addWidget(zoomDefaultsButton, 4, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(&compressionLabel);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addLayout(zoomLayout);
    mainLayout->addWidget(&multiresSectionLabel);
    mainLayout->addWidget(line);
    mainLayout->addWidget(lockDatasetCheckBox);
    mainLayout->addWidget(currentActiveMagDatasetLabel);
    mainLayout->addWidget(highestActiveMagDatasetLabel);
    mainLayout->addWidget(lowestActiveMagDatasetLabel);
    setLayout(mainLayout);

    connect(orthogonalDataViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(orthogonalSpinBoxChanged(double)));
    connect(skeletonViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxChanged(double)));

    connect(zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsClicked()));
    connect(lockDatasetCheckBox, SIGNAL(toggled(bool)), this, SLOT(lockDatasetMagChecked(bool)));

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetOptionsWidget::updateCompressionRatioDisplay() {
    switch(state->compressionRatio) {
    case 0:
        compressionLabel.setText("Current compression: none (toggle with (5))");
        break;
    case 1000:
        compressionLabel.setText("Current compression: jpg (toggle with (5))");
        break;
    case 1001:
        compressionLabel.setText("Current compression: j2k (toggle with (5))");
        break;
    default:
        compressionLabel.setText("Current compression: jp2 (toggle with (5))");
    }
}

/**
 * @brief DatasetOptionsWidget::orthogonalSpinBoxChanged  zooms in and out of ortho viewports with constant step.
 * @param value increment/decrement zoom by value
 */
void DatasetOptionsWidget::orthogonalSpinBoxChanged(double value) {
    state->viewer->window->forEachOrthoVPDo([&value](ViewportOrtho & orthoVP) {
        // conversion from percent to zoom level is inverted, because VPZOOMMAX < VPZOOMMIN,
        // because zoom level directly translates to displayed edge length
        float zoomLevel = 1 - value/100*VPZOOMMIN;
        if(zoomLevel < VPZOOMMAX) {
            zoomLevel = VPZOOMMAX;
        }
        orthoVP.texture.zoomLevel = zoomLevel;
    });
}
/**
 * @brief DatasetOptionsWidget::skeletonSpinBoxChanged increments or decrements the zoom. The step width is relative to
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
void DatasetOptionsWidget::skeletonSpinBoxChanged(double value) {
    if(userZoomSkel) {
        userZoomSkel = false;
        if ( ((value > lastZoomSkel && value < lastZoomSkel + 2)
                || (value < lastZoomSkel && value > lastZoomSkel - 2)) == false) {
            // difference at least greater than two,
            // so user entered a number instead of clicking the up and down buttons
            state->skeletonState->zoomLevel = value/100*SKELZOOMMAX;
        }
        else { // up or down button pressed, find out which.
            if(value > lastZoomSkel && value < lastZoomSkel + 2) { // user wants to zoom in
                state->viewer->window->viewport3D->zoomIn();
            }
            else if(value < lastZoomSkel && value > lastZoomSkel - 2) { // user wants to zoom out
                state->viewer->window->viewport3D->zoomOut();
            }
            // the following line will lead to signal emission and another call to this slot,
            // but since userZoomSkel was set to false above, no further recursion takes place.
            skeletonViewportSpinBox->setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);
        }
        lastZoomSkel = skeletonViewportSpinBox->value();
        userZoomSkel = true;
    }
}

void DatasetOptionsWidget::lockDatasetMagChecked(bool on) {
    state->viewerState->datasetMagLock = on;
}

/**
 * @brief DatasetOptionsWidget::zoomDefaultsClicked restores lowest zoom level on all viewports
 */
void DatasetOptionsWidget::zoomDefaultsClicked() {
    orthogonalDataViewportSpinBox->setValue(0);

    userZoomSkel = false;
    skeletonViewportSpinBox->setValue(100*SKELZOOMMIN/SKELZOOMMAX);
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    userZoomSkel = true;
}

void DatasetOptionsWidget::update() {
    orthogonalDataViewportLabel->setText(QString("Orthogonal Data Viewport (mag %1)").arg(state->magnification));
    float vpZoomPercent;
    if(state->viewer->vpUpperLeft->texture.zoomLevel == (float)VPZOOMMAX) {
        vpZoomPercent = 100;
    }
    else {
        vpZoomPercent = (1 - state->viewer->vpUpperLeft->texture.zoomLevel)*100;
    }
    orthogonalDataViewportSpinBox->setValue(vpZoomPercent);
    skeletonViewportSpinBox->setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);

    QString currentActiveMag = QString("Currently active mag dataset: %1").arg(state->magnification);
    QString highestActiveMag = QString("Highest available mag dataset: %1").arg(state->highestAvailableMag);
    QString lowestActiveMag = QString("Lowest available mag dataset: %1").arg(state->lowestAvailableMag);

    this->currentActiveMagDatasetLabel->setText(currentActiveMag);
    this->highestActiveMagDatasetLabel->setText(highestActiveMag);
    this->lowestActiveMagDatasetLabel->setText(lowestActiveMag);
}

void DatasetOptionsWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(ZOOM_AND_MULTIRES_WIDGET);
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
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

    QVariant datasetMagLock_value = settings.value(LOCK_DATASET_TO_CURRENT_MAG);
    this->lockDatasetCheckBox->setChecked(datasetMagLock_value.isNull() ?
                                                    LOCK_DATASET_ORIENTATION_DEFAULT :
                                                    datasetMagLock_value.toBool());
    emit(lockDatasetCheckBox->toggled(lockDatasetCheckBox->isChecked()));

    settings.endGroup();
    if(visible) {
        show();
    }
    else {
        hide();
    }
    setGeometry(x, y, width, height);
}

void DatasetOptionsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(ZOOM_AND_MULTIRES_WIDGET);
    settings.setValue(WIDTH, this->geometry().width());
    settings.setValue(HEIGHT, this->geometry().height());
    settings.setValue(POS_X, this->geometry().x());
    settings.setValue(POS_Y, this->geometry().y());
    settings.setValue(VISIBLE, this->isVisible());
    settings.setValue(ORTHO_DATA_VIEWPORTS, this->orthogonalDataViewportSpinBox->value());
    settings.setValue(SKELETON_VIEW, this->skeletonViewportSpinBox->value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, this->lockDatasetCheckBox->isChecked());
    settings.endGroup();
}



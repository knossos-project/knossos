/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2016
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
 *
 *
 *  For further information, visit http://www.knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "datasetoptionswidget.h"

#include "datasetloadwidget.h"
#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/viewport.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QPainter>
#include <QSettings>
#include <QSpacerItem>
#include <QStyle>

void ZoomSlider::paintEvent(QPaintEvent *ev) {
    // adapted from QCommonStyle::drawComplexControl case CC_Slider
    QStyleOptionSlider opt;
    initStyleOption(&opt);
    int thickness = style()->pixelMetric(QStyle::PM_SliderControlThickness, &opt, this);
    int len = style()->pixelMetric(QStyle::PM_SliderLength, &opt, this);
    int available = style()->pixelMetric(QStyle::PM_SliderSpaceAvailable, &opt, this);
    int interval = opt.tickInterval;
    if (interval <= 0) {
        interval = singleStep();
        if (QStyle::sliderPositionFromValue(opt.minimum, opt.maximum, interval, available)
            - QStyle::sliderPositionFromValue(opt.minimum, opt.maximum, 0, available) < 3)
            interval = opt.pageStep;
    }
    if (!interval) {
        interval = 1;
    }
    int fudge = len / 2;
    int pos;
    // Since there is no subrect for tickmarks do a translation here.
    QPainter painter(this);
    painter.save();
    painter.translate(opt.rect.x(), opt.rect.y());
    painter.setFont(QFont(painter.font().family(), 8 * devicePixelRatio()));
    painter.setPen(opt.palette.foreground().color());
    int v = opt.minimum;
    int drawnTicks = 0;
    while (v <= opt.maximum + 1 && drawnTicks < numTicks) {
        if (v == opt.maximum + 1 && interval == 1) {
            break;
        }
        QString label = QString::number(highestMag / std::pow(2, drawnTicks));
        drawnTicks++;
        const int v_ = qMin(v, opt.maximum);
        pos = QStyle::sliderPositionFromValue(opt.minimum, opt.maximum, v_, available) + fudge;
        painter.drawLine(pos, thickness / 2, pos,  thickness);
        painter.drawText(pos + interval/2 - QFontMetrics(painter.font()).width(label) / 2., thickness, label);
        // in the case where maximum is max int
        int nextInterval = v + interval;
        if (nextInterval < v) {
            break;
        }
        v = nextInterval;
    }
    painter.restore();
    QSlider::paintEvent(ev);
}

DatasetOptionsWidget::DatasetOptionsWidget(QWidget *parent, DatasetLoadWidget * datasetLoadWidget) :
    QDialog(parent), lastZoomSkel(0), userZoomSkel(true) {
    setWindowIcon(QIcon(":/resources/icons/zoom-in.png"));
    setWindowTitle("Dataset Options");

    updateCompressionRatioDisplay();

    // zoom section
    skeletonViewportSpinBox.setRange(0, 100);
    skeletonViewportSpinBox.setSuffix(" %");
    zoomDefaultsButton.setAutoDefault(false);

    orthoZoomSpinBox.setSuffix("%");
    orthoZoomSpinBox.setKeyboardTracking(false);
    orthoZoomSpinBox.setMinimum(1.);
    orthoZoomSpinBox.setSingleStep(100.);
    orthoZoomSlider.setOrientation(Qt::Horizontal);
    orthoZoomSlider.setSingleStep(1.);

    separator1.setFrameShape(QFrame::HLine);
    separator1.setFrameShadow(QFrame::Sunken);
    int row = 0;
    zoomLayout.addWidget(&zoomSectionLabel, row++, 0);
    zoomLayout.addWidget(&separator1, row++, 0, 1, 2);
    zoomLayout.addWidget(&orthogonalDataViewportLabel, row, 0);
    zoomLayout.addWidget(&orthoZoomSpinBox, row++, 1);
    zoomLayout.addWidget(&orthoZoomSlider, row++, 0, 1, 2);
    zoomLayout.addWidget(&skeletonViewportLabel, row, 0);
    zoomLayout.addWidget(&skeletonViewportSpinBox, row++, 1);
    zoomLayout.addWidget(&zoomDefaultsButton, row, 0, 1, 1, Qt::AlignLeft);

    currentActiveMagDatasetLabel.setText(tr("Currently active mag dataset: %1").arg(state->magnification));
    highestActiveMagDatasetLabel.setText(tr("Highest available mag dataset: %1").arg(state->highestAvailableMag));
    lowestActiveMagDatasetLabel.setText(tr("Lowest available mag dataset: %1").arg(state->lowestAvailableMag));

    mainLayout.addWidget(&compressionLabel);
    separator2.setFrameShape(QFrame::HLine);
    separator2.setFrameShadow(QFrame::Sunken);
    mainLayout.addLayout(&zoomLayout);
    mainLayout.addWidget(&multiresSectionLabel);
    mainLayout.addWidget(&separator2);
    mainLayout.addWidget(&lockDatasetCheckBox);
    mainLayout.addWidget(&currentActiveMagDatasetLabel);
    mainLayout.addWidget(&highestActiveMagDatasetLabel);
    mainLayout.addWidget(&lowestActiveMagDatasetLabel);
    setLayout(&mainLayout);

    connect(&orthoZoomSlider, &QSlider::valueChanged, [this](const int value) {
        const float newScreenPxXPerDataPx = state->viewer->lowestScreenPxXPerDataPx() * std::pow(zoomStep, value);
        const float prevFOV = state->viewer->viewportXY->texture.FOV;
        float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;

        if (state->viewerState->datasetMagLock == false) {
            if (prevFOV == VPZOOMMIN && static_cast<uint>(state->magnification) < state->viewer->highestMag() && prevFOV < newFOV) {
               state->viewer->updateDatasetMag(state->magnification * 2);
               newFOV = 0.5;
            }
            else if(prevFOV == 0.5 && static_cast<uint>(state->magnification) > state->viewer->lowestMag() && prevFOV > newFOV) {
                state->viewer->updateDatasetMag(state->magnification / 2);
                newFOV = VPZOOMMIN;
            } else {
                const float zoomMax = static_cast<float>(state->magnification) == state->lowestAvailableMag ? VPZOOMMAX : 0.5;
                newFOV = std::max(std::min(newFOV, static_cast<float>(VPZOOMMIN)), zoomMax);
            }
        }
        state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
            orthoVP.texture.FOV = newFOV;
        });
        state->viewer->recalcTextureOffsets();
        updateOrthogonalZoomSpinBox();
        updateOrthogonalZoomSlider();
    });

    connect(&orthoZoomSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) {
        const float newScreenPxXPerDataPx = (value / 100.) * state->viewer->lowestScreenPxXPerDataPx(false);
        if (!state->viewerState->datasetMagLock) {
            const uint newMag = state->viewer->calcMag(newScreenPxXPerDataPx);
            if (newMag != static_cast<uint>(state->magnification)) {
                state->viewer->updateDatasetMag(newMag);
            }
        }

        float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;

        state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
            orthoVP.texture.FOV = newFOV;
        });
        state->viewer->recalcTextureOffsets();
        orthoZoomSpinBox.blockSignals(true);
        orthoZoomSpinBox.setValue(100 * (newScreenPxXPerDataPx / state->viewer->lowestScreenPxXPerDataPx(false)));
        orthoZoomSpinBox.blockSignals(false);
        updateOrthogonalZoomSlider();
    });

    connect(&orthoZoomSlider, &QSlider::actionTriggered, [this](const int) {
        const int currSliderPos = orthoZoomSlider.sliderPosition();

        if (state->viewerState->datasetMagLock) {
            return;
        }

        int tickDistance = currSliderPos % orthoZoomSlider.tickInterval();
        const int snapDistance = 5;
        if (0 < tickDistance && tickDistance <= snapDistance) { // close over the next tick
            int newPos = currSliderPos - tickDistance;
            if (newPos <= (orthoZoomSlider.numTicks - 1) * orthoZoomSlider.tickInterval()) {
                orthoZoomSlider.setSliderPosition(newPos);
            }
        }
        else { // possibly close below the next tick
            int tickDistance = (orthoZoomSlider.sliderPosition() + snapDistance) % orthoZoomSlider.tickInterval();
            int newPos = (0 < tickDistance && tickDistance <= snapDistance) ? currSliderPos + snapDistance - tickDistance : currSliderPos;
            if (newPos <= (orthoZoomSlider.numTicks - 1) * orthoZoomSlider.tickInterval()) {
                orthoZoomSlider.setSliderPosition(newPos);
            }
        }
    });

    connect(&skeletonViewportSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) {
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
                skeletonViewportSpinBox.setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);
            }
            lastZoomSkel = skeletonViewportSpinBox.value();
            userZoomSkel = true;
        }
    });

    connect(&zoomDefaultsButton, &QPushButton::clicked, this, &DatasetOptionsWidget::zoomDefaultsClicked);

    connect(&lockDatasetCheckBox, &QCheckBox::toggled, [this] (const bool on) {
        state->viewer->setMagnificationLock(on);
    });
    connect(state->viewer, &Viewer::magnificationLockChanged, [this](const bool locked){
        const auto signalsBlocked = lockDatasetCheckBox.blockSignals(true);
        lockDatasetCheckBox.setChecked(locked);
        lockDatasetCheckBox.blockSignals(signalsBlocked);
        reinitializeOrthoZoomWidgets();
    });
    connect(datasetLoadWidget, &DatasetLoadWidget::datasetChanged, this, &DatasetOptionsWidget::reinitializeOrthoZoomWidgets);

    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetOptionsWidget::applyZoom(const float newScreenPxXPerDataPx) {
    const float prevFOV = state->viewer->viewportXY->texture.FOV;
    float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;

    if (newFOV >= 1 && static_cast<uint>(state->magnification) < state->viewer->highestMag() && prevFOV < std::round(newFOV)) {
       state->viewer->updateDatasetMag(state->magnification * 2);
       newFOV = 0.5;
    }
    else if(newFOV <= 0.5 && static_cast<uint>(state->magnification) > state->viewer->lowestMag() && prevFOV > std::round(newFOV)) {
        state->viewer->updateDatasetMag(state->magnification / 2);
        newFOV = 1.;
    }
    state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
        orthoVP.texture.FOV = newFOV;
    });

    state->viewer->recalcTextureOffsets();
}

void DatasetOptionsWidget::reinitializeOrthoZoomWidgets() {
    const auto mags = int_log(state->viewer->highestMag()) - int_log(state->viewer->lowestMag()) + 1;
    const auto interval = 50;

    zoomStep = std::pow(2, 1./interval);
    int max_value = std::log(state->viewer->highestScreenPxXPerDataPx() / state->viewer->lowestScreenPxXPerDataPx()) / std::log(zoomStep);
    orthoZoomSlider.numTicks = mags > 1 ? mags : 0;
    orthoZoomSlider.highestMag = state->viewer->highestMag();
    const auto orthoZoomSliderBlock = orthoZoomSlider.blockSignals(true);
    orthoZoomSlider.setRange(0, max_value);
    orthoZoomSlider.setTickInterval(interval);
    updateOrthogonalZoomSlider();
    orthoZoomSlider.blockSignals(orthoZoomSliderBlock);

    const auto orthoZoomSpinBoxBlock = orthoZoomSpinBox.blockSignals(true);
    orthoZoomSpinBox.setMinimum(100 * (state->viewer->lowestScreenPxXPerDataPx(false) / state->viewer->lowestScreenPxXPerDataPx(false)));
    orthoZoomSpinBox.setMaximum(100 * (state->viewer->highestScreenPxXPerDataPx(false) / state->viewer->lowestScreenPxXPerDataPx(false)));
    updateOrthogonalZoomSpinBox();
    orthoZoomSlider.blockSignals(orthoZoomSpinBoxBlock);
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

void DatasetOptionsWidget::updateOrthogonalZoomSpinBox() {
    orthoZoomSpinBox.blockSignals(true);
    const double newValue = state->viewer->viewportXY->screenPxXPerDataPx / state->viewer->lowestScreenPxXPerDataPx(false);
    orthoZoomSpinBox.setValue(newValue * 100);
    orthoZoomSpinBox.blockSignals(false);
}

void DatasetOptionsWidget::updateOrthogonalZoomSlider() {
    orthoZoomSlider.blockSignals(true);
    const int newValue = std::log(state->viewer->viewportXY->screenPxXPerDataPx / state->viewer->lowestScreenPxXPerDataPx()) / std::log(zoomStep);
    orthoZoomSlider.setValue(newValue);
    orthoZoomSlider.blockSignals(false);
}

void DatasetOptionsWidget::zoomDefaultsClicked() {
    state->viewer->zoomReset();
    userZoomSkel = false;
    skeletonViewportSpinBox.setValue(100 * SKELZOOMMIN/SKELZOOMMAX);
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    userZoomSkel = true;
}

void DatasetOptionsWidget::update() {
    updateOrthogonalZoomSlider();
    updateOrthogonalZoomSpinBox();
    skeletonViewportSpinBox.setValue(100*state->skeletonState->zoomLevel/SKELZOOMMAX);

    currentActiveMagDatasetLabel.setText(tr("Currently active mag dataset: %1").arg(state->magnification));
    highestActiveMagDatasetLabel.setText(tr("Highest available mag dataset: %1").arg(state->highestAvailableMag));
    lowestActiveMagDatasetLabel.setText(tr("Lowest available mag dataset: %1").arg(state->lowestAvailableMag));
}

void DatasetOptionsWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_OPTIONS_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());
    setVisible(settings.value(VISIBLE, false).toBool());

    auto skeletonZoom = settings.value(SKELETON_VIEW, 0).toDouble();
    if(skeletonZoom > 0) {
        userZoomSkel = false;
        skeletonViewportSpinBox.setValue(skeletonZoom);
        userZoomSkel = true;
        lastZoomSkel = skeletonViewportSpinBox.value();
        state->skeletonState->zoomLevel = 0.01*lastZoomSkel * SKELZOOMMAX;
    } else {
        skeletonViewportSpinBox.setValue(0);
    }

    auto datasetMagLockValue = settings.value(LOCK_DATASET_TO_CURRENT_MAG, false).toBool();
    lockDatasetCheckBox.setChecked(datasetMagLockValue);
    emit lockDatasetCheckBox.toggled(lockDatasetCheckBox.isChecked());

    settings.endGroup();
}

void DatasetOptionsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(DATASET_OPTIONS_WIDGET);
    settings.setValue(GEOMETRY, saveGeometry());
    settings.setValue(VISIBLE, isVisible());
    settings.setValue(SKELETON_VIEW, skeletonViewportSpinBox.value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, lockDatasetCheckBox.isChecked());
    settings.endGroup();
}

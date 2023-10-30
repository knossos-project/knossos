/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
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
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#include "zoomwidget.h"

#include "dataset.h"
#include "datasetloadwidget.h"
#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/viewports/viewportbase.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QPainter>
#include <QSettings>
#include <QSignalBlocker>
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
    painter.setFont(QFont(painter.font().family(), 8 * devicePixelRatioF()));
    painter.setPen(opt.palette.windowText().color());
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
        painter.drawText(pos + interval/2 - QFontMetrics(painter.font()).horizontalAdvance(label) / 2., thickness, label);
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

ZoomWidget::ZoomWidget(QWidget *parent, DatasetLoadWidget * datasetLoadWidget)
        : DialogVisibilityNotify(ZOOM_WIDGET, parent), lastZoomSkel(0), userZoomSkel(true) {
    setWindowIcon(QIcon(":/resources/icons/zoom.png"));
    setWindowTitle("Zoom");

    // zoom section
    skeletonViewportSpinBox.setRange(SKELZOOMMIN * 100, SKELZOOMMAX * 100);
    skeletonViewportSpinBox.setSuffix(" %");
    zoomDefaultsButton.setAutoDefault(false);

    orthoZoomSpinBox.setSuffix("%");
    orthoZoomSpinBox.setKeyboardTracking(false);
    orthoZoomSpinBox.setMinimum(1.);
    orthoZoomSpinBox.setSingleStep(100.);
    orthoZoomSlider.setOrientation(Qt::Horizontal);
    orthoZoomSlider.setSingleStep(1.);

    int row = 0;
    zoomLayout.addWidget(&orthogonalDataViewportLabel, row, 0);
    zoomLayout.addWidget(&orthoZoomSpinBox, row++, 1);
    zoomLayout.addWidget(&orthoZoomSlider, row++, 0, 1, 2);
    zoomLayout.addWidget(&skeletonViewportLabel, row, 0);
    zoomLayout.addWidget(&skeletonViewportSpinBox, row++, 1);
    zoomLayout.addWidget(&zoomDefaultsButton, row, 0, 1, 1, Qt::AlignLeft);

    currentActiveMagDatasetLabel.setText(tr("Currently active: mag%1").arg(Dataset::current().toMag(Dataset::current().magIndex)));
    highestActiveMagDatasetLabel.setText(tr("Highest available: mag%1").arg(Dataset::current().toMag(Dataset::current().highestAvailableMagIndex)));
    lowestActiveMagDatasetLabel.setText(tr("Lowest available: mag%1").arg(Dataset::current().toMag(Dataset::current().lowestAvailableMagIndex)));

    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);
    mainLayout.addLayout(&zoomLayout);
    mainLayout.addWidget(&multiresSectionLabel);
    mainLayout.addWidget(&separator);
    mainLayout.addWidget(&lockDatasetCheckBox);
    mainLayout.addWidget(&currentActiveMagDatasetLabel);
    mainLayout.addWidget(&highestActiveMagDatasetLabel);
    mainLayout.addWidget(&lowestActiveMagDatasetLabel);
    setLayout(&mainLayout);

    connect(&orthoZoomSlider, &QSlider::valueChanged, [this](const int value) {
        const float newScreenPxXPerDataPx = state->viewer->lowestScreenPxXPerDataPx() * std::pow(zoomStep, value);
        state->viewer->zoom(newScreenPxXPerDataPx);
    });

    connect(&orthoZoomSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) {
        const float newScreenPxXPerDataPx = (value / 100.) * state->viewer->lowestScreenPxXPerDataPx();
        state->viewer->zoom(newScreenPxXPerDataPx);
    });

    connect(&orthoZoomSlider, &QSlider::actionTriggered, [](const int) {
//        const int currSliderPos = orthoZoomSlider.sliderPosition();

//        if (state->viewerState->datasetMagLock) {
//            return;
//        }

//        int tickDistance = currSliderPos % orthoZoomSlider.tickInterval();
//        const int snapDistance = 5;
//        if (0 < tickDistance && tickDistance <= snapDistance) { // close over the next tick
//            int newPos = currSliderPos - tickDistance;
//            if (newPos <= (orthoZoomSlider.numTicks - 1) * orthoZoomSlider.tickInterval()) {
//                orthoZoomSlider.setSliderPosition(newPos);
//            }
//        }
//        else { // possibly close below the next tick
//            int tickDistance = (orthoZoomSlider.sliderPosition() + snapDistance) % orthoZoomSlider.tickInterval();
//            int newPos = (0 < tickDistance && tickDistance <= snapDistance) ? currSliderPos + snapDistance - tickDistance : currSliderPos;
//            if (newPos <= (orthoZoomSlider.numTicks - 1) * orthoZoomSlider.tickInterval()) {
//                orthoZoomSlider.setSliderPosition(newPos);
//            }
//        }
    });

    connect(&skeletonViewportSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) {
        if(userZoomSkel) {
            userZoomSkel = false;
            if ( ((value > lastZoomSkel && value < lastZoomSkel + 2)
                    || (value < lastZoomSkel && value > lastZoomSkel - 2)) == false) {
                // difference at least greater than two,
                // so user entered a number instead of clicking the up and down buttons
                state->mainWindow->viewport3D->zoomFactor = value/100;
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
                skeletonViewportSpinBox.setValue(100 * state->mainWindow->viewport3D->zoomFactor);
            }
            lastZoomSkel = skeletonViewportSpinBox.value();
            userZoomSkel = true;
        }
    });

    connect(&zoomDefaultsButton, &QPushButton::clicked, this, &ZoomWidget::zoomDefaultsClicked);

    connect(&lockDatasetCheckBox, &QCheckBox::toggled, [] (const bool on) {
        state->viewer->setMagnificationLock(on);
    });
    QObject::connect(state->viewer, &Viewer::zoomChanged, this, &ZoomWidget::update);
    connect(state->viewer, &Viewer::magnificationLockChanged, [this](const bool locked){
        QSignalBlocker blocker{lockDatasetCheckBox};
        lockDatasetCheckBox.setChecked(locked);
        reinitializeOrthoZoomWidgets();
    });
    connect(datasetLoadWidget, &DatasetLoadWidget::datasetChanged, this, &ZoomWidget::reinitializeOrthoZoomWidgets);
}

void ZoomWidget::reinitializeOrthoZoomWidgets() {
    if (Dataset::datasets.empty()) {
        return;
    }
    const auto mags = Dataset::datasets[0].highestAvailableMagIndex  - Dataset::datasets[0].lowestAvailableMagIndex + 1;
    const auto interval = 50;

    zoomStep = std::pow(2, 1./interval);
    int max_value = std::ceil(std::log(state->viewer->highestScreenPxXPerDataPx() / state->viewer->lowestScreenPxXPerDataPx()) / std::log(zoomStep));
    orthoZoomSlider.numTicks = mags > 1 ? mags : 0;
    orthoZoomSlider.highestMag = Dataset::datasets[0].highestAvailableMagIndex;
    QSignalBlocker blockerSlider{orthoZoomSlider};
    orthoZoomSlider.setRange(0, max_value);
    orthoZoomSlider.setTickInterval(interval);
    updateOrthogonalZoomSlider();

    QSignalBlocker blockerSpin{orthoZoomSpinBox};
    orthoZoomSpinBox.setMinimum(100 * (state->viewer->lowestScreenPxXPerDataPx() / state->viewer->lowestScreenPxXPerDataPx()));
    orthoZoomSpinBox.setMaximum(100 * (state->viewer->highestScreenPxXPerDataPx() / state->viewer->lowestScreenPxXPerDataPx()));
    updateOrthogonalZoomSpinBox();
}

void ZoomWidget::updateOrthogonalZoomSpinBox() {
    QSignalBlocker blocker{orthoZoomSpinBox};
    const double newValue = state->viewer->viewportXY->screenPxXPerDataPx / state->viewer->lowestScreenPxXPerDataPx();
    orthoZoomSpinBox.setValue(newValue * 100);
}

void ZoomWidget::updateOrthogonalZoomSlider() {
    QSignalBlocker blocker{orthoZoomSlider};
    const int newValue = std::ceil(std::log(state->viewer->viewportXY->screenPxXPerDataPx / state->viewer->lowestScreenPxXPerDataPx()) / std::log(zoomStep));
    orthoZoomSlider.setValue(newValue);
}

void ZoomWidget::zoomDefaultsClicked() {
    state->viewer->zoomReset();
    userZoomSkel = false;
    skeletonViewportSpinBox.setValue(100);
    state->mainWindow->viewport3D->zoomFactor = 1;
    userZoomSkel = true;
}

void ZoomWidget::update() {
    if (Dataset::datasets.empty()) {
        return;
    }
    updateOrthogonalZoomSlider();
    updateOrthogonalZoomSpinBox();
    skeletonViewportSpinBox.setValue(100 * state->mainWindow->viewport3D->zoomFactor);

    currentActiveMagDatasetLabel.setText(tr("Currently active: mag%1").arg(Dataset::current().toMag(Dataset::current().magIndex)));
    highestActiveMagDatasetLabel.setText(tr("Highest available: mag%1").arg(Dataset::current().toMag(Dataset::current().highestAvailableMagIndex)));
    lowestActiveMagDatasetLabel.setText(tr("Lowest available: mag%1").arg(Dataset::current().toMag(Dataset::current().lowestAvailableMagIndex)));
}

void ZoomWidget::loadSettings() {
    QSettings settings;
    settings.beginGroup(ZOOM_WIDGET);
    restoreGeometry(settings.value(GEOMETRY).toByteArray());

    auto skeletonZoom = settings.value(SKELETON_VIEW, 0).toDouble();
    if(skeletonZoom > 0) {
        userZoomSkel = false;
        skeletonViewportSpinBox.setValue(skeletonZoom);
        userZoomSkel = true;
        lastZoomSkel = skeletonViewportSpinBox.value();
        state->mainWindow->viewport3D->zoomFactor = lastZoomSkel / 100;
    } else {
        skeletonViewportSpinBox.setValue(0);
    }

    auto datasetMagLockValue = settings.value(LOCK_DATASET_TO_CURRENT_MAG, false).toBool();
    lockDatasetCheckBox.setChecked(datasetMagLockValue);
    emit lockDatasetCheckBox.toggled(lockDatasetCheckBox.isChecked());

    settings.endGroup();
}

void ZoomWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(ZOOM_WIDGET);
    settings.setValue(VISIBLE, isVisible());
    settings.setValue(SKELETON_VIEW, skeletonViewportSpinBox.value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, lockDatasetCheckBox.isChecked());
    settings.endGroup();
}

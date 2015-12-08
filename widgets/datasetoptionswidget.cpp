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

#include "datasetloadwidget.h"
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
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QSpacerItem>
#include <QApplication>
#include <QDesktopWidget>

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
    skeletonViewportLabel = new QLabel("Skeleton Viewport");
    skeletonViewportSpinBox = new QDoubleSpinBox();
    skeletonViewportSpinBox->setMaximum(100);
    skeletonViewportSpinBox->setMinimum(0);
    skeletonViewportSpinBox->setSuffix(" %");
    zoomDefaultsButton = new QPushButton("All Zoom defaults");
    zoomDefaultsButton->setAutoDefault(false);

    orthoZoomSpinBox.setSuffix("%");
    orthoZoomSpinBox.setKeyboardTracking(false);
    orthoZoomSpinBox.setMinimum(1.);
    orthoZoomSpinBox.setSingleStep(100.);
    orthoZoomSlider.setOrientation(Qt::Horizontal);
    orthoZoomSlider.setSingleStep(1.);

    // multires section
    lockDatasetCheckBox = new QCheckBox("Lock dataset to current mag");
    currentActiveMagDatasetLabel = new QLabel(QString("Currently active mag dataset: %1").arg(state->magnification));
    highestActiveMagDatasetLabel = new QLabel(QString("Highest available mag dataset: %1").arg(state->highestAvailableMag));
    lowestActiveMagDatasetLabel = new QLabel(QString("Lowest available mag dataset: %1").arg(state->lowestAvailableMag));

    QGridLayout *zoomLayout = new QGridLayout();
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    int row = 0;
    zoomLayout->addWidget(&zoomSectionLabel, row++, 0);
    zoomLayout->addWidget(line, row++, 0, 1, 2);
    zoomLayout->addWidget(&orthogonalDataViewportLabel, row, 0);
    zoomLayout->addWidget(&orthoZoomSpinBox, row++, 1);
    zoomLayout->addWidget(&orthoZoomSlider, row++, 0, 1, 2);
    zoomLayout->addWidget(skeletonViewportLabel, row, 0);
    zoomLayout->addWidget(skeletonViewportSpinBox, row++, 1);
    zoomLayout->addWidget(zoomDefaultsButton, row, 0);

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

    connect(&orthoZoomSlider, &QSlider::valueChanged, [this](const int value) {
        const float newScreenPxXPerDataPx = lowestScreenPxXPerDataPx() * std::pow(zoomStep, value);
        const float prevFOV = state->viewer->viewportXY->texture.FOV;
        float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;

        if (prevFOV == VPZOOMMIN && static_cast<uint>(state->magnification) < highestMag() && prevFOV < newFOV) {
           state->viewer->updateDatasetMag(state->magnification * 2);
           newFOV = 0.5;
        }
        else if(prevFOV == 0.5 && static_cast<uint>(state->magnification) > lowestMag() && prevFOV > newFOV) {
            state->viewer->updateDatasetMag(state->magnification / 2);
            newFOV = VPZOOMMIN;
        } else {
            const float zoomMax = static_cast<float>(state->magnification) == state->lowestAvailableMag ? VPZOOMMAX : 0.5;
            newFOV = std::max(std::min(newFOV, static_cast<float>(VPZOOMMIN)), zoomMax);
        }
        state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
            orthoVP.texture.FOV = newFOV;
        });

        state->viewer->recalcTextureOffsets();
        updateOrthogonalZoomSpinBox();
        updateOrthogonalZoomSlider();
    });

    connect(&orthoZoomSpinBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double value) {
        const float newScreenPxXPerDataPx = (value / 100.) * lowestScreenPxXPerDataPx(false);
        if (lockDatasetCheckBox->isChecked() == false) {
            const uint newMag = calcMag(newScreenPxXPerDataPx);
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
        orthoZoomSpinBox.setValue(100 * (newScreenPxXPerDataPx / lowestScreenPxXPerDataPx(false)));
        orthoZoomSpinBox.blockSignals(false);
        updateOrthogonalZoomSlider();
    });

    connect(&orthoZoomSlider, &QSlider::actionTriggered, [this](const int) {
        const int currSliderPos = orthoZoomSlider.sliderPosition();

        if (lockDatasetCheckBox->isChecked()) {
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

    connect(skeletonViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxChanged(double)));

    connect(zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsClicked()));

    QObject::connect(lockDatasetCheckBox, &QCheckBox::toggled, [this] (const bool on) {
        state->viewerState->datasetMagLock = on;
        if (!on) {
            const uint newMag = calcMag(state->viewer->viewportXY->screenPxXPerDataPx);
            if (newMag != static_cast<uint>(state->magnification)) {
                state->viewer->updateDatasetMag(newMag);
                float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / state->viewer->viewportXY->screenPxXPerDataPx;
                state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
                    orthoVP.texture.FOV = newFOV;
                });
            }
        }
        orthoZoomSlider.blockSignals(true);
        orthoZoomSpinBox.blockSignals(true);
        reinitializeOrthoZoomWidgets();
        orthoZoomSpinBox.blockSignals(false);
        orthoZoomSlider.blockSignals(false);
    });

    QObject::connect(datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](const bool) {
        reinitializeOrthoZoomWidgets();
    });

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void DatasetOptionsWidget::applyZoom(const float newScreenPxXPerDataPx) {
    const float prevFOV = state->viewer->viewportXY->texture.FOV;
    float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;

    if (newFOV >= 1 && static_cast<uint>(state->magnification) < highestMag() && prevFOV < std::round(newFOV)) {
       state->viewer->updateDatasetMag(state->magnification * 2);
       newFOV = 0.5;
    }
    else if(newFOV <= 0.5 && static_cast<uint>(state->magnification) > lowestMag() && prevFOV > std::round(newFOV)) {
        state->viewer->updateDatasetMag(state->magnification / 2);
        newFOV = 1.;
    }
    state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
        orthoVP.texture.FOV = newFOV;
    });

    state->viewer->recalcTextureOffsets();
}

void DatasetOptionsWidget::reinitializeOrthoZoomWidgets() {
    const auto mags = int_log(highestMag()) - int_log(lowestMag()) + 1;
    const auto interval = 50;

    zoomStep = std::pow(2, 1./interval);
    int max_value = std::log(highestScreenPxXPerDataPx() / lowestScreenPxXPerDataPx()) / std::log(zoomStep);
    orthoZoomSlider.numTicks = mags > 1 ? mags : 0;
    orthoZoomSlider.highestMag = highestMag();
    orthoZoomSlider.setMinimum(0);
    orthoZoomSlider.setMaximum(max_value);
    orthoZoomSlider.setTickInterval(interval);
    updateOrthogonalZoomSlider();

    orthoZoomSpinBox.setMinimum(100 * (lowestScreenPxXPerDataPx(false) / lowestScreenPxXPerDataPx(false)));
    orthoZoomSpinBox.setMaximum(100 * (highestScreenPxXPerDataPx(false) / lowestScreenPxXPerDataPx(false)));
    updateOrthogonalZoomSpinBox();
}

uint DatasetOptionsWidget::highestMag() {
    return lockDatasetCheckBox->isChecked() ? state->magnification : state->highestAvailableMag;
}

uint DatasetOptionsWidget::lowestMag() {
    return lockDatasetCheckBox->isChecked() ? state->magnification : state->lowestAvailableMag;
}

float DatasetOptionsWidget::highestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / state->viewerState->texEdgeLength / (ofCurrentMag ? lowestMag() : state->lowestAvailableMag);
    auto * vp = state->viewer->viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * VPZOOMMAX * state->cubeEdgeLength) / vp->texture.edgeLengthPx;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

float DatasetOptionsWidget::lowestScreenPxXPerDataPx(const bool ofCurrentMag) {
    const float texUnitsPerDataPx = 1. / state->viewerState->texEdgeLength / (ofCurrentMag ? highestMag() : state->highestAvailableMag);
    auto * vp = state->viewer->viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * state->cubeEdgeLength) / vp->texture.edgeLengthPx;
    displayedEdgeLen = (std::ceil(displayedEdgeLen / 2. / texUnitsPerDataPx) * texUnitsPerDataPx) * 2.;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
}

uint DatasetOptionsWidget::calcMag(const float screenPxXPerDataPx) {
    const float exactMag = state->highestAvailableMag * lowestScreenPxXPerDataPx() / screenPxXPerDataPx;
    const uint roundedPower = std::ceil(std::log(exactMag) / std::log(2));
    return std::min(state->highestAvailableMag, std::max(static_cast<uint>(std::pow(2, roundedPower)), state->lowestAvailableMag));
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
    const double newValue = state->viewer->viewportXY->screenPxXPerDataPx / lowestScreenPxXPerDataPx(false);
    orthoZoomSpinBox.setValue(newValue * 100);
    orthoZoomSpinBox.blockSignals(false);
}

void DatasetOptionsWidget::updateOrthogonalZoomSlider() {
    orthoZoomSlider.blockSignals(true);
    const int newValue = std::log(state->viewer->viewportXY->screenPxXPerDataPx / lowestScreenPxXPerDataPx()) / std::log(zoomStep);
    orthoZoomSlider.setValue(newValue);
    orthoZoomSlider.blockSignals(false);
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

/**
 * @brief DatasetOptionsWidget::zoomDefaultsClicked restores lowest zoom level on all viewports
 */
void DatasetOptionsWidget::zoomDefaultsClicked() {
    state->viewer->zoomReset();
    userZoomSkel = false;
    skeletonViewportSpinBox->setValue(100*SKELZOOMMIN/SKELZOOMMAX);
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    userZoomSkel = true;
}

void DatasetOptionsWidget::update() {
    updateOrthogonalZoomSlider();
    updateOrthogonalZoomSpinBox();
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
    settings.setValue(SKELETON_VIEW, this->skeletonViewportSpinBox->value());
    settings.setValue(LOCK_DATASET_TO_CURRENT_MAG, this->lockDatasetCheckBox->isChecked());
    settings.endGroup();
}



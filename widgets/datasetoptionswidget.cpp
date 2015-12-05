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
    int tickOffset = style()->pixelMetric(QStyle::PM_SliderTickmarkOffset, &opt, this);
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
        QString label = QString::number(state->highestAvailableMag / std::pow(2, drawnTicks));
        drawnTicks++;
        const int v_ = qMin(v, opt.maximum);
        pos = QStyle::sliderPositionFromValue(opt.minimum, opt.maximum, v_, available) + fudge;
        painter.drawLine(pos, 0, pos,  thickness / 2);
        painter.drawText(pos - QFontMetrics(painter.font()).width(label) / 2. + 1, thickness, label);
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
    orthogonalDataViewportLabel = new QLabel(QString("Orthogonal Data Viewport"));
    skeletonViewportLabel = new QLabel("Skeleton Viewport");
    skeletonViewportSpinBox = new QDoubleSpinBox();
    skeletonViewportSpinBox->setMaximum(100);
    skeletonViewportSpinBox->setMinimum(0);
    skeletonViewportSpinBox->setSuffix(" %");
    zoomDefaultsButton = new QPushButton("All Zoom defaults");
    zoomDefaultsButton->setAutoDefault(false);

    orthogonalZoomSlider.setOrientation(Qt::Horizontal);
    orthogonalZoomSlider.setSingleStep(1.);

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
    zoomLayout->addWidget(&orthogonalZoomSlider, 2, 1);
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

    connect(&orthogonalZoomSlider, &QSlider::valueChanged, [this](const int value) {
        const float newScreenPxXPerDataPx = lowestScreenPxXPerDataPx() * std::pow(zoomStep, value);
        const float prevFOV = state->viewer->viewportXY->texture.FOV;
        float newFOV = state->viewer->viewportXY->screenPxXPerDataPxForZoomFactor(1.f) / newScreenPxXPerDataPx;
        if (newFOV >= 1 && static_cast<uint>(state->magnification) < state->highestAvailableMag && prevFOV < std::round(newFOV)) {
           state->viewer->changeDatasetMag(MAG_UP);
           newFOV = 0.5;
        }
        else if(newFOV <= 0.5 && static_cast<uint>(state->magnification) > state->lowestAvailableMag && prevFOV > std::round(newFOV)) {
            state->viewer->changeDatasetMag(MAG_DOWN);
            newFOV = 1.;
        }
        state->viewer->window->forEachOrthoVPDo([&newFOV](ViewportOrtho & orthoVP) {
            orthoVP.texture.FOV = newFOV;
        });
        state->viewer->recalcTextureOffsets();
    });

    connect(&orthogonalZoomSlider, &QSlider::actionTriggered, [this](const int) {
        const int currSliderPos = orthogonalZoomSlider.sliderPosition();

        if (lockDatasetCheckBox->isChecked()) {
            const int magNo = int_log(state->highestAvailableMag) - int_log(state->magnification);
            const int tickMin = magNo * orthogonalZoomSlider.tickInterval();
            const int tickMax = magNo * orthogonalZoomSlider.tickInterval() + orthogonalZoomSlider.tickInterval() - 1;
            if (currSliderPos > tickMax) {
                orthogonalZoomSlider.setSliderPosition(tickMax);
            }
            else if (currSliderPos < tickMin) {
                orthogonalZoomSlider.setSliderPosition(tickMin);
            }
            return;
        }

        int tickDistance = currSliderPos % orthogonalZoomSlider.tickInterval();
        const int snapDistance = 5;
        if (0 < tickDistance && tickDistance <= snapDistance) { // possibly close over the next tick
            orthogonalZoomSlider.setSliderPosition(currSliderPos - tickDistance);
        }
        else { // possibly close below the next tick
            int tickDistance = (orthogonalZoomSlider.sliderPosition() + snapDistance) % orthogonalZoomSlider.tickInterval();
            if (0 < tickDistance && tickDistance <= snapDistance) {
                orthogonalZoomSlider.setSliderPosition(currSliderPos + snapDistance - tickDistance);
            }
            else {
                orthogonalZoomSlider.setSliderPosition(currSliderPos);
            }
        }
    });

    connect(skeletonViewportSpinBox, SIGNAL(valueChanged(double)), this, SLOT(skeletonSpinBoxChanged(double)));

    connect(zoomDefaultsButton, SIGNAL(clicked()), this, SLOT(zoomDefaultsClicked()));
    connect(lockDatasetCheckBox, SIGNAL(toggled(bool)), this, SLOT(lockDatasetMagChecked(bool)));

    QObject::connect(datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](const bool) {
        const auto mags = int_log(state->highestAvailableMag) - int_log(state->lowestAvailableMag) + 1;
        const auto interval = 50;

        zoomStep = std::pow(2, 1./interval);
        orthogonalZoomSlider.numTicks = mags;
        orthogonalZoomSlider.setMinimum(0);
        orthogonalZoomSlider.setMaximum(mags*interval);
        orthogonalZoomSlider.setTickInterval(interval);
        updateOrthogonalZoomSlider();
    });


    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

float DatasetOptionsWidget::lowestScreenPxXPerDataPx() {
    const float texUnitsPerDataPx = 1. / state->viewerState->texEdgeLength / state->highestAvailableMag;
    auto * vp = state->viewer->viewportXY;
    float FOVinDCs = static_cast<float>(state->M) - 1.f;
    float displayedEdgeLen = (FOVinDCs * state->cubeEdgeLength) / vp->texture.edgeLengthPx;
    return vp->edgeLength / (displayedEdgeLen / texUnitsPerDataPx);
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

void DatasetOptionsWidget::updateOrthogonalZoomSlider() {
    orthogonalZoomSlider.blockSignals(true);
    int newValue = std::log(state->viewer->viewportXY->screenPxXPerDataPx / lowestScreenPxXPerDataPx()) / std::log(zoomStep);
    orthogonalZoomSlider.setValue(newValue);
    orthogonalZoomSlider.blockSignals(false);
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
    state->viewer->zoomReset();
    userZoomSkel = false;
    skeletonViewportSpinBox->setValue(100*SKELZOOMMIN/SKELZOOMMAX);
    state->skeletonState->zoomLevel = SKELZOOMMIN;
    userZoomSkel = true;
}

void DatasetOptionsWidget::update() {
    orthogonalDataViewportLabel->setText(QString("Orthogonal Viewports"));
    updateOrthogonalZoomSlider();
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



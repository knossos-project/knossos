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
 *  For further information, visit https://knossostool.org
 *  or contact knossos-team@mpimf-heidelberg.mpg.de
 */

#include "datasetsegmentationtab.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"
#include "segmentation/segmentation.h"
#include "viewer.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>

DatasetAndSegmentationTab::DatasetAndSegmentationTab(QWidget *parent) : QWidget(parent) {
    biasSpinBox.setRange(0, 255);
    biasSlider.setRange(0, 255);

    rangeDeltaSlider.setRange(1, 255);
    rangeDeltaSpinBox.setRange(1, 255);

    overlayGroup.setCheckable(true);
    segmentationOverlaySpinBox.setRange(0, 255);
    segmentationOverlaySlider.setRange(0, 255);

    volumeColorButton.setStyleSheet("background-color : " + Segmentation::singleton().volume_background_color.name() + ";");
    volumeOpaquenessSpinBox.setRange(0, 255);
    volumeOpaquenessSlider.setRange(0, 255);

    // dataset
    int row = 0;
    datasetLayout.addWidget(&datasetLinearFilteringCheckBox, row++, 0, 1, 2);
    datasetLayout.addWidget(&useOwnDatasetColorsCheckBox, row, 0); datasetLayout.addWidget(&useOwnDatasetColorsButton, row++, 1, Qt::AlignLeft);
    datasetLayout.addWidget(&biasLabel, row, 0); datasetLayout.addWidget(&biasSlider, row, 1); datasetLayout.addWidget(&biasSpinBox, row++, 2);
    datasetLayout.addWidget(&rangeDeltaLabel, row, 0); datasetLayout.addWidget(&rangeDeltaSlider, row, 1); datasetLayout.addWidget(&rangeDeltaSpinBox, row++, 2);
    datasetLayout.setAlignment(Qt::AlignTop);
    datasetGroup.setLayout(&datasetLayout);
    // segmentation
    overlayLayout.addWidget(&segmentationOverlayLabel);
    overlayLayout.addWidget(&segmentationOverlaySlider);
    overlayLayout.addWidget(&segmentationOverlaySpinBox);
    overlayGroup.setLayout(&overlayLayout);
    row = 0;
    segmentationLayout.addWidget(&overlayGroup, row++, 0, 1, 3);
    segmentationLayout.addWidget(&volumeRenderCheckBox, row++, 0, 1, 2);
    segmentationLayout.addWidget(&volumeOpaquenessLabel, row, 0); segmentationLayout.addWidget(&volumeOpaquenessSlider, row, 1); segmentationLayout.addWidget(&volumeOpaquenessSpinBox, row++, 2);
    segmentationLayout.addWidget(&volumeColorLabel, row, 0); segmentationLayout.addWidget(&volumeColorButton, row++, 1, Qt::AlignLeft);
    segmentationLayout.setAlignment(Qt::AlignTop);
    segmentationGroup.setLayout(&segmentationLayout);
    mainLayout.addWidget(&datasetGroup);
    mainLayout.addWidget(&segmentationGroup);
    setLayout(&mainLayout);

    QObject::connect(&datasetLinearFilteringCheckBox, &QCheckBox::clicked, [](const bool checked) {
        if (checked) {
            state->viewer->applyTextureFilterSetting(GL_LINEAR);
        } else {
            state->viewer->applyTextureFilterSetting(GL_NEAREST);
        }
    });
    QObject::connect(&useOwnDatasetColorsCheckBox, &QCheckBox::clicked, [this](const bool checked) {
        if (checked) {//load file if none is cached
            useOwnDatasetColorsButtonClicked(lutFilePath);
        } else {
            state->viewer->defaultDatasetLUT();
        }
    });
    QObject::connect(&useOwnDatasetColorsButton, &QPushButton::clicked, [this](){ useOwnDatasetColorsButtonClicked(); });
    QObject::connect(&biasSlider, &QSlider::valueChanged, [this](const int value) {
        state->viewerState->luminanceBias = value;
        biasSpinBox.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&biasSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        state->viewerState->luminanceBias = value;
        biasSlider.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&rangeDeltaSlider, &QSlider::valueChanged, [this](const int value) {
        state->viewerState->luminanceRangeDelta = value;
        rangeDeltaSpinBox.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&rangeDeltaSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        state->viewerState->luminanceRangeDelta = value;
        rangeDeltaSlider.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });

    QObject::connect(state->viewer, &Viewer::layerVisibilityChanged, [this](const int index) {
        if (index == 1) {
            overlayGroup.setChecked(state->viewerState->layerVisibility.at(1));
        }
    });
    QObject::connect(&overlayGroup, &QGroupBox::clicked, [this](const bool checked) { state->viewerState->layerVisibility.at(1) = checked; });
    QObject::connect(&segmentationOverlaySlider, &QSlider::valueChanged, [this](int value){
        segmentationOverlaySpinBox.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    QObject::connect(&segmentationOverlaySpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value){
        segmentationOverlaySlider.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    QObject::connect(&volumeRenderCheckBox, &QCheckBox::clicked, [this](bool checked){
        Segmentation::singleton().volume_render_toggle = checked;
        volumeOpaquenessLabel.setEnabled(checked);
        volumeOpaquenessSlider.setEnabled(checked);
        volumeOpaquenessSpinBox.setEnabled(checked);
        volumeColorLabel.setEnabled(checked);
        volumeColorButton.setEnabled(checked);
        emit volumeRenderToggled();
    });
    QObject::connect(&volumeColorButton, &QPushButton::clicked, [this]() {
        state->viewerState->renderInterval = SLOW;
        auto color = QColorDialog::getColor(Segmentation::singleton().volume_background_color, this, "Select background color");
        state->viewerState->renderInterval = FAST;
        if (color.isValid() == QColorDialog::Accepted) {
            Segmentation::singleton().volume_background_color = color;
            volumeColorButton.setStyleSheet("background-color: " + color.name() + ";");
        }
    });
    QObject::connect(&volumeOpaquenessSlider, &QSlider::valueChanged, [this](int value){
        volumeOpaquenessSpinBox.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });
    QObject::connect(&volumeOpaquenessSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value){
        volumeOpaquenessSlider.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });

    QObject::connect(&state->viewer->mainWindow, &MainWindow::overlayOpacityChanged, [this]() { segmentationOverlaySlider.setValue(Segmentation::singleton().alpha); });
}

void DatasetAndSegmentationTab::useOwnDatasetColorsButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, "Load Dataset Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            state->viewer->loadDatasetLUT(path);
            lutFilePath = path;
            useOwnDatasetColorsCheckBox.setChecked(true);
        } catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, "LUT loading failed", "LUTs are restricted to 256 RGB tuples", QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            useOwnDatasetColorsCheckBox.setChecked(false);
        }
    } else {
        useOwnDatasetColorsCheckBox.setChecked(false);
    }
}

void DatasetAndSegmentationTab::saveSettings() const {
    QSettings settings;
    settings.beginGroup(PREFERENCES_WIDGET);
    settings.setValue(DATASET_LINEAR_FILTERING, datasetLinearFilteringCheckBox.isChecked());
    settings.setValue(BIAS, biasSpinBox.value());
    settings.setValue(RANGE_DELTA, rangeDeltaSpinBox.value());
    settings.setValue(SEGMENTATION_OVERLAY_ALPHA, segmentationOverlaySlider.value());
    settings.setValue(DATASET_LUT_FILE, lutFilePath);
    settings.setValue(DATASET_LUT_FILE_USED, useOwnDatasetColorsCheckBox.isChecked());
    settings.setValue(RENDER_VOLUME, volumeRenderCheckBox.isChecked());
    settings.setValue(VOLUME_ALPHA, volumeOpaquenessSpinBox.value());
    settings.setValue(VOLUME_BACKGROUND_COLOR, Segmentation::singleton().volume_background_color);
}

void DatasetAndSegmentationTab::loadSettings() {
    QSettings settings;
    settings.beginGroup(PREFERENCES_WIDGET);
    datasetLinearFilteringCheckBox.setChecked(settings.value(DATASET_LINEAR_FILTERING, true).toBool());
    datasetLinearFilteringCheckBox.clicked(datasetLinearFilteringCheckBox.isChecked());
    lutFilePath = settings.value(DATASET_LUT_FILE, "").toString();
    // again, load the path-string first, before populating the checkbox
    useOwnDatasetColorsCheckBox.setChecked(settings.value(DATASET_LUT_FILE_USED, false).toBool());
    useOwnDatasetColorsCheckBox.clicked(useOwnDatasetColorsCheckBox.isChecked());
    biasSpinBox.setValue(settings.value(BIAS, 0).toInt());
    biasSpinBox.valueChanged(biasSpinBox.value());
    rangeDeltaSpinBox.setValue(settings.value(RANGE_DELTA, 255).toInt());
    rangeDeltaSpinBox.valueChanged(rangeDeltaSpinBox.value());
    segmentationOverlaySlider.setValue(settings.value(SEGMENTATION_OVERLAY_ALPHA, 37).toInt());
    segmentationOverlaySlider.valueChanged(segmentationOverlaySlider.value());
    volumeRenderCheckBox.setChecked(settings.value(RENDER_VOLUME, false).toBool());
    volumeRenderCheckBox.clicked(volumeRenderCheckBox.isChecked());
    volumeOpaquenessSpinBox.setValue(settings.value(VOLUME_ALPHA, 37).toInt());
    volumeOpaquenessSpinBox.valueChanged(volumeOpaquenessSpinBox.value());
    Segmentation::singleton().volume_background_color = settings.value(VOLUME_BACKGROUND_COLOR, QColor(Qt::darkGray)).value<QColor>();
    volumeColorButton.setStyleSheet("background-color: " + Segmentation::singleton().volume_background_color.name() + ";");
}

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

#include "datasetsegmentationtab.h"

#include "gui_wrapper.h"
#include "segmentation/segmentation.h"
#include "stateInfo.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"
#include "widgets/mainwindow.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>

DatasetAndSegmentationTab::DatasetAndSegmentationTab(QWidget *parent) : QWidget(parent) {
    datasetLinearFilteringCheckBox.setToolTip("Sets linear filtering for all non overlay layers \nand resets to nearest filtering for all overlay layers.");

    biasSpinBox.setRange(0, 255);
    biasSlider.setRange(0, 255);

    rangeDeltaSlider.setRange(-255, 255);
    rangeDeltaSlider.setTickInterval(255);
    rangeDeltaSlider.setTickPosition(QSlider::TicksBothSides);
    rangeDeltaSpinBox.setRange(-255, 255);

    createPaintObjectCheck.setToolTip(tr("If there is no Object selected, \nautomatically create a fresh Object \nwhose Subobject ID is used for painting."));
    overlayGroup.setCheckable(true);
    segmentationOverlaySpinBox.setRange(0, 255);
    segmentationOverlaySlider.setRange(0, 255);

    overrideObjectColorCombo.addItems({"Class color", "Subobject color"});

    volumeGroup.setCheckable(true);
    volumeGroup.setChecked(false);

    volumeColorButton.setStyleSheet("background-color : " + Segmentation::singleton().volume_background_color.name() + ";");
    volumeOpaquenessSpinBox.setRange(0, 255);
    volumeOpaquenessSlider.setRange(0, 255);

    segmentationBorderHighlight.setCheckable(true);

    // dataset
    int row = 0;
    datasetLayout.addWidget(&datasetLinearFilteringCheckBox, row++, 0, 1, 2);
    datasetLayout.addWidget(&useOwnDatasetColorsCheckBox, row, 0); datasetLayout.addWidget(&useOwnDatasetColorsButton, row++, 1, Qt::AlignLeft);
    datasetLayout.addWidget(&biasLabel, row, 0); datasetLayout.addWidget(&biasSlider, row, 1); datasetLayout.addWidget(&biasSpinBox, row++, 2);
    datasetLayout.addWidget(&rangeDeltaLabel, row, 0); datasetLayout.addWidget(&rangeDeltaSlider, row, 1); datasetLayout.addWidget(&rangeDeltaSpinBox, row++, 2);
    datasetLayout.setAlignment(Qt::AlignTop);
    datasetGroup.setLayout(&datasetLayout);
    // segmentation
    row = 0;
    overlayLayout.addWidget(&segmentationOverlayLabel, row, 0);
    overlayLayout.addWidget(&segmentationOverlaySlider, row, 1);
    overlayLayout.addWidget(&segmentationOverlaySpinBox, row, 2);
    overlayLayout.addWidget(&segmentationBorderHighlight, ++row, 0);
    overlayLayout.addWidget(&overrideObjectColorCheck, ++row, 0);
    overlayLayout.addWidget(&overrideObjectColorCombo, row, 1);
    overlayGroup.setLayout(&overlayLayout);
    row = 0;
    volumeLayout.addWidget(&volumeOpaquenessLabel, row, 0); volumeLayout.addWidget(&volumeOpaquenessSlider, row, 1); volumeLayout.addWidget(&volumeOpaquenessSpinBox, row, 2);
    volumeLayout.addWidget(&volumeColorLabel, ++row, 0); volumeLayout.addWidget(&volumeColorButton, row, 1, Qt::AlignLeft);
    volumeGroup.setLayout(&volumeLayout);

    segmentationLayout.addWidget(&createPaintObjectCheck);
    segmentationLayout.addWidget(&overlayGroup);
    segmentationLayout.addWidget(&volumeGroup);
    segmentationLayout.setAlignment(Qt::AlignTop);
    segmentationGroup.setLayout(&segmentationLayout);
    mainLayout.addWidget(&datasetGroup);
    mainLayout.addWidget(&segmentationGroup);
    setLayout(&mainLayout);

    QObject::connect(&datasetLinearFilteringCheckBox, &QCheckBox::clicked, [](const bool checked) {
        state->viewer->applyTextureFilterSetting(checked ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
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
        state->viewerState->luminanceBias = static_cast<double>(value) / biasSpinBox.maximum();
        biasSpinBox.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&biasSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        state->viewerState->luminanceBias = static_cast<double>(value) / biasSlider.maximum();
        biasSlider.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&rangeDeltaSlider, &QSlider::valueChanged, [this](const int value) {
        state->viewerState->luminanceRangeDelta = static_cast<double>(value) / rangeDeltaSpinBox.maximum();
        rangeDeltaSpinBox.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });
    QObject::connect(&rangeDeltaSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int value) {
        state->viewerState->luminanceRangeDelta = static_cast<double>(value) / rangeDeltaSlider.maximum();
        rangeDeltaSlider.setValue(value);
        state->viewer->datasetColorAdjustmentsChanged();
    });

    QObject::connect(&createPaintObjectCheck, &QCheckBox::toggled, [](bool state){
        Segmentation::singleton().createPaintObject = state;
    });
    QObject::connect(state->viewer, &Viewer::layerVisibilityChanged, this, [this](const std::size_t index) {
        if (index == Segmentation::singleton().layerId) {
            overlayGroup.setChecked(Dataset::datasets.at(index).renderSettings.visible);
        }
    });
    QObject::connect(&overlayGroup, &QGroupBox::clicked, [](const bool checked) { state->viewer->setLayerVisibility(Segmentation::singleton().layerId, checked); });
    QObject::connect(&segmentationOverlaySlider, &QSlider::valueChanged, [this](int value){
        segmentationOverlaySpinBox.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->segmentation_changed();
    });
    QObject::connect(&segmentationOverlaySpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value){
        segmentationOverlaySlider.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->segmentation_changed();
    });
    QObject::connect(&segmentationBorderHighlight, &QCheckBox::clicked, [](const bool checked) {
        Segmentation::singleton().highlightBorder = checked;
        state->viewer->segmentation_changed();
    });
    QObject::connect(&overrideObjectColorCheck, &QCheckBox::toggled, [this](const bool override) {
        Segmentation::singleton().segmentationColor = !override ? SegmentationColor::Object : static_cast<SegmentationColor>(overrideObjectColorCombo.currentIndex());
        overrideObjectColorCombo.setEnabled(override);
        state->viewer->segmentation_changed();
    });
    QObject::connect(&overrideObjectColorCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int index) {
        qDebug() << "CURRENT INDEX CHANGED";
        Segmentation::singleton().segmentationColor = static_cast<SegmentationColor>(index);
        state->viewer->segmentation_changed();
    });

    QObject::connect(&volumeGroup, &QGroupBox::clicked, &Segmentation::singleton(), &Segmentation::toggleVolumeRender);
    QObject::connect(&Segmentation::singleton(), &Segmentation::volumeRenderToggled, &volumeGroup, &QGroupBox::setChecked);

    QObject::connect(&volumeColorButton, &QPushButton::clicked, [this]() {
        const auto color = state->viewer->suspend([this]{
            return QColorDialog::getColor(Segmentation::singleton().volume_background_color, this, "Select background color");
        });
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

    QObject::connect(state->mainWindow, &MainWindow::overlayOpacityChanged, [this](){
        segmentationOverlaySlider.setValue(Segmentation::singleton().alpha);
    });
    QObject::connect(&state->mainWindow->widgetContainer.datasetLoadWidget, &DatasetLoadWidget::datasetChanged, [this](){
        segmentationGroup.setEnabled(Segmentation::singleton().enabled);
        overlayGroup.setChecked(Segmentation::singleton().enabled);
        if (!Segmentation::singleton().enabled) {
            volumeGroup.setChecked(false);
            volumeGroup.clicked(false);
        }
    });
}

void DatasetAndSegmentationTab::useOwnDatasetColorsButtonClicked(QString path) {
    if (path.isEmpty()) {
        path = state->viewer->suspend([this]{
            return QFileDialog::getOpenFileName(this, "Load Dataset Color Lookup Table", QDir::homePath(), tr("LUT file (*.lut *.json)"));
        });
    }
    useOwnDatasetColorsCheckBox.setChecked(false);// if it fails
    if (!path.isEmpty()) {//load LUT and apply
        try {
            state->viewer->loadDatasetLUT(path);
            lutFilePath = path;
            useOwnDatasetColorsCheckBox.setChecked(true);
        } catch (...) {
            loadLutError(path);
        }
    }
}

void DatasetAndSegmentationTab::saveSettings() const {
    QSettings settings;
    settings.beginGroup(PREFERENCES_WIDGET);
    settings.setValue(DATASET_LINEAR_FILTERING, datasetLinearFilteringCheckBox.isChecked());
    settings.setValue(BIAS, biasSpinBox.value());
    settings.setValue(RANGE_DELTA, rangeDeltaSpinBox.value());
    settings.setValue(SEGMENTATION_CREATE_PAINT_OBJECT, createPaintObjectCheck.isChecked());
    settings.setValue(SEGMENTATION_OVERLAY_ALPHA, segmentationOverlaySlider.value());
    settings.setValue(SEGMENTATITION_HIGHLIGHT_BORDER, segmentationBorderHighlight.isChecked());
    settings.setValue(SEGMENTATION_COLOR, overrideObjectColorCheck.isChecked() ? overrideObjectColorCombo.currentIndex() : static_cast<int>(SegmentationColor::Object));
    settings.setValue(DATASET_LUT_FILE, lutFilePath);
    settings.setValue(DATASET_LUT_FILE_USED, useOwnDatasetColorsCheckBox.isChecked());
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
    createPaintObjectCheck.setChecked(settings.value(SEGMENTATION_CREATE_PAINT_OBJECT, Segmentation::singleton().createPaintObject).toBool());
    segmentationOverlaySlider.setValue(settings.value(SEGMENTATION_OVERLAY_ALPHA, 37).toInt());
    segmentationOverlaySlider.valueChanged(segmentationOverlaySlider.value());
    segmentationBorderHighlight.setChecked(settings.value(SEGMENTATITION_HIGHLIGHT_BORDER, true).toBool());
    segmentationBorderHighlight.clicked(segmentationBorderHighlight.isChecked());
    const auto segmentationColorIdx = settings.value(SEGMENTATION_COLOR, 0).toInt();
    if (static_cast<SegmentationColor>(segmentationColorIdx) != SegmentationColor::Object) {
        overrideObjectColorCheck.setChecked(true);
        overrideObjectColorCombo.setCurrentIndex(segmentationColorIdx);
        overrideObjectColorCombo.currentIndexChanged(segmentationColorIdx);
    }
    volumeOpaquenessSpinBox.setValue(settings.value(VOLUME_ALPHA, 37).toInt());
    volumeOpaquenessSpinBox.valueChanged(volumeOpaquenessSpinBox.value());
    Segmentation::singleton().volume_background_color = settings.value(VOLUME_BACKGROUND_COLOR, QColor(Qt::darkGray)).value<QColor>();
    volumeColorButton.setStyleSheet("background-color: " + Segmentation::singleton().volume_background_color.name() + ";");
}

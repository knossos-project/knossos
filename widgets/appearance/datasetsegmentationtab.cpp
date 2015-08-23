#include "datasetsegmentationtab.h"

#include "segmentation/segmentation.h"
#include "viewer.h"

#include <QMessageBox>

DatasetAndSegmentationTab::DatasetAndSegmentationTab(QWidget *parent) : QWidget(parent)
{
    biasSpinBox.setMaximum(255);
    biasSpinBox.setSingleStep(1);
    biasSlider.setMaximum(255);
    biasSlider.setSingleStep(1);

    rangeDeltaSlider.setMaximum(255);
    rangeDeltaSlider.setSingleStep(1);
    rangeDeltaSpinBox.setMaximum(255);
    rangeDeltaSpinBox.setSingleStep(1);

    segmentationOverlaySpinBox.setMaximum(255);
    segmentationOverlaySpinBox.setSingleStep(1);
    segmentationOverlaySlider.setMaximum(255);
    segmentationOverlaySlider.setSingleStep(1);

    volumeColorButton.setStyleSheet("background-color : " + Segmentation::singleton().volume_background_color.name() + ";");
    volumeOpaquenessSpinBox.setMaximum(255);
    volumeOpaquenessSpinBox.setSingleStep(1);
    volumeOpaquenessSlider.setMaximum(255);
    volumeOpaquenessSlider.setSingleStep(1);

    datasetSeparator.setFrameShape(QFrame::HLine);
    datasetSeparator.setFrameShadow(QFrame::Sunken);
    segmentationSeparator.setFrameShape(QFrame::HLine);
    segmentationSeparator.setFrameShadow(QFrame::Sunken);

    int row = 0;
    mainLayout.setAlignment(Qt::AlignTop);
    // dataset
    mainLayout.addWidget(&datasetHeader, row++, 0);
    mainLayout.addWidget(&datasetSeparator, row++, 0, 1, 3);
    mainLayout.addWidget(&datasetLinearFilteringCheckBox, row++, 0, 1, 2);
    mainLayout.addWidget(&useOwnDatasetColorsCheckBox, row, 0); mainLayout.addWidget(&useOwnDatasetColorsButton, row++, 1, Qt::AlignLeft);
    mainLayout.addWidget(&biasLabel, row, 0); mainLayout.addWidget(&biasSlider, row, 1); mainLayout.addWidget(&biasSpinBox, row++, 2);
    mainLayout.addWidget(&rangeDeltaLabel, row, 0); mainLayout.addWidget(&rangeDeltaSlider, row, 1); mainLayout.addWidget(&rangeDeltaSpinBox, row++, 2);
    // segmentation overlay
    mainLayout.addWidget(&segmentationHeader, row++, 0);
    mainLayout.addWidget(&segmentationSeparator, row++, 0, 1, 3);
    mainLayout.addWidget(&segmentationOverlayLabel, row, 0); mainLayout.addWidget(&segmentationOverlaySlider, row, 1); mainLayout.addWidget(&segmentationOverlaySpinBox, row++, 2);
    // segmentation volume
    mainLayout.addWidget(&volumeRenderCheckBox, row++, 0, 1, 2);
    mainLayout.addWidget(&volumeOpaquenessLabel, row, 0); mainLayout.addWidget(&volumeOpaquenessSlider, row, 1); mainLayout.addWidget(&volumeOpaquenessSpinBox, row++, 2);
    mainLayout.addWidget(&volumeColorLabel, row, 0); mainLayout.addWidget(&volumeColorButton, row++, 1, Qt::AlignLeft);
    setLayout(&mainLayout);

    QObject::connect(&datasetLinearFilteringCheckBox, &QCheckBox::toggled, [](const bool checked) {
        if (checked) {
            state->viewer->applyTextureFilterSetting(GL_LINEAR);
        } else {
            state->viewer->applyTextureFilterSetting(GL_NEAREST);
        }
    });
    QObject::connect(&useOwnDatasetColorsCheckBox, &QCheckBox::toggled, [this](const bool checked) {
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
    QObject::connect(&segmentationOverlaySlider, &QSlider::valueChanged, [&](int value){
        segmentationOverlaySpinBox.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    QObject::connect(&segmentationOverlaySpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
        segmentationOverlaySlider.setValue(value);
        Segmentation::singleton().alpha = value;
        state->viewer->oc_reslice_notify_visible();
    });
    QObject::connect(&volumeRenderCheckBox, &QCheckBox::toggled, [this](bool checked){
        Segmentation::singleton().volume_render_toggle = checked;
        emit volumeRenderToggled();
    });
    QObject::connect(&volumeColorButton, &QPushButton::clicked, [&](){
        auto color = QColorDialog::getColor(Segmentation::singleton().volume_background_color, this, "Select background color");
        if (color.isValid() == QColorDialog::Accepted) {
            Segmentation::singleton().volume_background_color = color;
            volumeColorButton.setStyleSheet("background-color: " + color.name() + ";");
        }
    });
    QObject::connect(&volumeOpaquenessSlider, &QSlider::valueChanged, [&](int value){
        volumeOpaquenessSpinBox.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });
    QObject::connect(&volumeOpaquenessSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&](int value){
        volumeOpaquenessSlider.setValue(value);
        Segmentation::singleton().volume_opacity = value;
        Segmentation::singleton().volume_update_required = true;
    });
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

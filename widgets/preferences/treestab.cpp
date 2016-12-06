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

#include "treestab.h"
#include "widgets/GuiConstants.h"
#include "viewer.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>

TreesTab::TreesTab(QWidget *parent) : QWidget(parent) {
    depthCutoffSpin.setSingleStep(0.5);
    depthCutoffSpin.setMinimum(0.5);

    renderQualityCombo.addItems({tr("High (slow)"), tr("Default (very fast)"), tr("Low (fastest)")});
    renderQualityCombo.setToolTip(tr("<b>Skeleton rendering quality:</b><br/>"
                                  "<b>High:</b> Highest level of detail possible, but slower performance, especially for big skeletons. <br/>"
                                  "<b>Default:</b> Dynamical adjustment of details depending on zoom level. <br/>"
                                  "<b>Low:</b> Skeleton rendered as lines and points. Details decrease drastically when zoomed out. Very fast even for large skeletons. <br/>"));

    // trees
    renderingLayout.setAlignment(Qt::AlignTop);
    renderingLayout.addRow(&highlightActiveTreeCheck);
    renderingLayout.addRow(&highlightIntersectionsCheck);
    renderingLayout.addRow(&lightEffectsCheck);
    renderingLayout.addRow(tr("MSAA"), &msaaSpin);
    renderingLayout.addRow(&ownTreeColorsCheck, &loadTreeLUTButton);
    renderingLayout.addRow(tr("Depth cutoff:"), &depthCutoffSpin);
    renderingLayout.addRow(tr("Skeleton rendering quality:"), &renderQualityCombo);
    renderingGroup.setLayout(&renderingLayout);
    visibilityLayout.setAlignment(Qt::AlignTop);
    visibilityLayout.addWidget(&skeletonInOrthoVPsCheck);
    visibilityLayout.addWidget(&skeletonIn3DVPCheck);
    visibilityLayout.addWidget(&wholeSkeletonRadio);
    visibilityLayout.addWidget(&selectedTreesRadio);
    visibilityGroup.setLayout(&visibilityLayout);

    mainLayout.addWidget(&renderingGroup);
    mainLayout.addWidget(&visibilityGroup);
    setLayout(&mainLayout);

    // trees render options
    QObject::connect(&highlightActiveTreeCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->highlightActiveTree = on; });
    QObject::connect(&highlightIntersectionsCheck, &QCheckBox::clicked, [this](const bool checked) {
        state->viewerState->showIntersections = checked;
    });
    QObject::connect(&lightEffectsCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->lightOnOff = on; });
    QObject::connect(&msaaSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [](const int samples){
        if (samples != state->viewerState->sampleBuffers) {
            state->viewerState->sampleBuffers = samples;
            state->viewer->saveSettings();
            state->viewer->window->widgetContainer.preferencesWidget.datasetAndSegmentationTab.saveSettings();
            state->viewer->window->widgetContainer.datasetOptionsWidget.saveSettings();
            state->mainWindow->createViewports();
            state->viewer->loadSettings();
            state->viewer->window->widgetContainer.datasetOptionsWidget.loadSettings();
            state->viewer->window->widgetContainer.preferencesWidget.datasetAndSegmentationTab.loadSettings();
            state->mainWindow->forEachVPDo([](ViewportBase & vp) {
                if (vp.isDocked == false && state->mainWindow->isVisible()) {
                    // this is true if user changes msaa settings but false if KNOSSOS was just launched.
                    vp.setDock(false);
                }
            });
        }
    });
    QObject::connect(&ownTreeColorsCheck, &QCheckBox::clicked, [this](const bool checked) {
        if (checked) {//load file if none is cached
            loadTreeLUTButtonClicked(lutFilePath);
        } else {
            state->viewer->loadTreeLUT();
        }
    });
    QObject::connect(&loadTreeLUTButton, &QPushButton::clicked, [this]() { loadTreeLUTButtonClicked(); });
    QObject::connect(&depthCutoffSpin, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [](const double value) { state->viewerState->depthCutOff = value; });
    QObject::connect(&renderQualityCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [](const int value){
        if(value == 0) { // tubes and spheres
            state->viewerState->cumDistRenderThres = 1.f;
        } else if(value == 1) { // switch dynamically
            state->viewerState->cumDistRenderThres = 7.f;
        } else { // lines and points
            state->viewerState->cumDistRenderThres = 20.f;
        }
    });

    // tree visibility
    QObject::connect(&wholeSkeletonRadio, &QRadioButton::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&selectedTreesRadio, &QRadioButton::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonInOrthoVPsCheck, &QCheckBox::clicked, this, &TreesTab::updateTreeDisplay);
    QObject::connect(&skeletonIn3DVPCheck, &QCheckBox::clicked, this, &TreesTab::updateTreeDisplay);

    createGlobalAction(Qt::CTRL + Qt::Key_T, [this](){// T for trees
        if (wholeSkeletonRadio.isChecked()) {
            selectedTreesRadio.setChecked(true);
        } else {
            wholeSkeletonRadio.setChecked(true);
        }
        wholeSkeletonRadio.clicked(wholeSkeletonRadio.isChecked());
        selectedTreesRadio.clicked(selectedTreesRadio.isChecked());
    });
}

void TreesTab::updateTreeDisplay() {
    state->viewerState->skeletonDisplay = 0x0;
    if (selectedTreesRadio.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::OnlySelected;
    }
    if (skeletonIn3DVPCheck.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::ShowIn3DVP;
    }
    if (skeletonInOrthoVPsCheck.isChecked()) {
        state->viewerState->skeletonDisplay |= SkeletonDisplay::ShowInOrthoVPs;
    }
}

void TreesTab::loadTreeLUTButtonClicked(QString path) {
    if (path.isEmpty()) {
        state->viewerState->renderInterval = SLOW;
        path = QFileDialog::getOpenFileName(this, tr("Load Tree Color Lookup Table"), QDir::homePath(), tr("LUT file (*.lut *.json)"));
        state->viewerState->renderInterval = FAST;
    }
    if (!path.isEmpty()) {//load LUT and apply
        try {
            state->viewer->loadTreeLUT(path);
            lutFilePath = path;
            ownTreeColorsCheck.setChecked(true);
        }  catch (...) {
            QMessageBox lutErrorBox(QMessageBox::Warning, tr("LUT loading failed"), tr("LUTs are restricted to 256 RGB tuples"), QMessageBox::Ok, this);
            lutErrorBox.setDetailedText(tr("Path: %1").arg(path));
            lutErrorBox.exec();
            ownTreeColorsCheck.setChecked(false);
        }
    } else {
        ownTreeColorsCheck.setChecked(false);
    }
}

void TreesTab::saveSettings(QSettings & settings) const {
    settings.setValue(LIGHT_EFFECTS, lightEffectsCheck.isChecked());
    settings.setValue(MSAA_SAMPLES, msaaSpin.value());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, highlightActiveTreeCheck.isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, highlightIntersectionsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE_USED, ownTreeColorsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE, lutFilePath);
    settings.setValue(DEPTH_CUTOFF, depthCutoffSpin.value());
    settings.setValue(RENDERING_QUALITY, renderQualityCombo.currentIndex());
    settings.setValue(WHOLE_SKELETON, wholeSkeletonRadio.isChecked());
    settings.setValue(ONLY_SELECTED_TREES, selectedTreesRadio.isChecked());
    settings.setValue(SHOW_SKELETON_ORTHOVPS, skeletonInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_SKELETON_SKELVP, skeletonIn3DVPCheck.isChecked());
}

void TreesTab::loadSettings(const QSettings & settings) {
    lightEffectsCheck.setChecked(settings.value(LIGHT_EFFECTS, true).toBool());
    lightEffectsCheck.clicked(lightEffectsCheck.isChecked());
    msaaSpin.setValue(settings.value(MSAA_SAMPLES, 8).toInt());
    msaaSpin.valueChanged(msaaSpin.value());
    highlightActiveTreeCheck.setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE, true).toBool());
    highlightActiveTreeCheck.clicked(highlightActiveTreeCheck.isChecked());
    highlightIntersectionsCheck.setChecked(settings.value(HIGHLIGHT_INTERSECTIONS, false).toBool());
    highlightIntersectionsCheck.clicked(highlightIntersectionsCheck.isChecked());
    depthCutoffSpin.setValue(settings.value(DEPTH_CUTOFF, 5.).toDouble());
    depthCutoffSpin.valueChanged(depthCutoffSpin.value());
    auto renderQuality = settings.value(RENDERING_QUALITY, 1).toInt();
    renderQualityCombo.setCurrentIndex(renderQuality);
    if (renderQualityCombo.currentIndex() == -1) {
        renderQualityCombo.setCurrentIndex(1);
    }
    renderQualityCombo.currentIndexChanged(renderQuality);
    lutFilePath = settings.value(TREE_LUT_FILE, "").toString();
    //it’s impotant to populate the checkbox after loading the path-string, because emitted signals depend on the lut // TODO VP settings: is that true?
    ownTreeColorsCheck.setChecked(settings.value(TREE_LUT_FILE_USED, false).toBool());
    ownTreeColorsCheck.clicked(ownTreeColorsCheck.isChecked());
    wholeSkeletonRadio.setChecked(settings.value(WHOLE_SKELETON, true).toBool());
    wholeSkeletonRadio.clicked(wholeSkeletonRadio.isChecked());
    selectedTreesRadio.setChecked(settings.value(ONLY_SELECTED_TREES, false).toBool());
    selectedTreesRadio.clicked(selectedTreesRadio.isChecked());
    skeletonInOrthoVPsCheck.setChecked(settings.value(SHOW_SKELETON_ORTHOVPS, true).toBool());
    skeletonInOrthoVPsCheck.clicked(skeletonInOrthoVPsCheck.isChecked());
    skeletonIn3DVPCheck.setChecked(settings.value(SHOW_SKELETON_SKELVP, true).toBool());
    skeletonIn3DVPCheck.clicked(skeletonIn3DVPCheck.isChecked());
}

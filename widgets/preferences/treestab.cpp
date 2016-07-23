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

#include "treestab.h"
#include "widgets/GuiConstants.h"
#include "viewer.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>

TreesTab::TreesTab(QWidget *parent) : QWidget(parent) {
    depthCutoffSpin.setSingleStep(0.5);
    depthCutoffSpin.setMinimum(0.5);

    renderQualityCombo.addItems({tr("Tubes & spheres"), tr("Switch dynamically"), tr("Lines & points")});
    renderQualityCombo.setToolTip(tr("<b>Skeleton rendering quality:</b><br/>"
                                  "<b>Tubes & spheres:</b> renders segments as tubes and nodes as spheres<br/>"
                                  "<b>Switch dynamically:</b> Switches dynamically between \"Tubes & spheres\" and \"Lines & points\" depending on zoom<br/>"
                                  "<b>Lines & points:</b> renders segments as lines and nodes as points<br/>"));

    auto treeDisplayLayout = new QVBoxLayout();
    treeDisplayLayout->addWidget(&skeletonInOrthoVPsCheck);
    treeDisplayLayout->addWidget(&skeletonIn3DVPCheck);
    treeDisplayLayout->addWidget(&wholeSkeletonRadio);
    treeDisplayLayout->addWidget(&selectedTreesRadio);

    auto row = 0;
    // trees
    mainLayout.setAlignment(Qt::AlignTop);
    mainLayout.addWidget(&highlightActiveTreeCheck, row, 0);  mainLayout.addLayout(treeDisplayLayout, row++, 3, 4, 1);
    mainLayout.addWidget(&highlightIntersectionsCheck, row++, 0);
    mainLayout.addWidget(&lightEffectsCheck, row++, 0);
    mainLayout.addWidget(&ownTreeColorsCheck, row, 0);  mainLayout.addWidget(&loadTreeLUTButton, row++, 1);
    mainLayout.addWidget(&depthCutOffLabel, row, 0);  mainLayout.addWidget(&depthCutoffSpin, row++, 1);
    mainLayout.addWidget(&renderQualityLabel, row, 0);  mainLayout.addWidget(&renderQualityCombo, row++, 1);

    setLayout(&mainLayout);

    // trees render options
    QObject::connect(&highlightActiveTreeCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->highlightActiveTree = on; });
    QObject::connect(&highlightIntersectionsCheck, &QCheckBox::clicked, [this](const bool checked) {
        state->viewerState->showIntersections = checked;
    });
    QObject::connect(&lightEffectsCheck, &QCheckBox::clicked, [](const bool on) { state->viewerState->lightOnOff = on; });
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
        path = QFileDialog::getOpenFileName(this, tr("Load Tree Color Lookup Table"), QDir::homePath(), tr("LUT file (*.lut *.json)"));
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
    highlightActiveTreeCheck.setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE, true).toBool());
    highlightActiveTreeCheck.clicked(highlightActiveTreeCheck.isChecked());
    highlightIntersectionsCheck.setChecked(settings.value(HIGHLIGHT_INTERSECTIONS, false).toBool());
    highlightIntersectionsCheck.clicked(highlightIntersectionsCheck.isChecked());
    depthCutoffSpin.setValue(settings.value(DEPTH_CUTOFF, 5.).toDouble());
    depthCutoffSpin.valueChanged(depthCutoffSpin.value());
    renderQualityCombo.setCurrentIndex(settings.value(RENDERING_QUALITY, 1).toInt());
    if (renderQualityCombo.currentIndex() == -1) {
        renderQualityCombo.setCurrentIndex(1);
    }
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

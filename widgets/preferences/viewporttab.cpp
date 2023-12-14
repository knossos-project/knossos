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

#include "stateInfo.h"
#include "viewporttab.h"
#include "viewer.h"
#include "widgets/GuiConstants.h"
#include "widgets/viewports/viewportbase.h"

ViewportTab::ViewportTab(QWidget *parent) : QWidget(parent) {
    showScalebarCheckBox.setTristate();
    boundaryGroup.addButton(&boundariesPixelRadioBtn);
    boundaryGroup.addButton(&boundariesPhysicalRadioBtn);

    rotationCenterGroup.addButton(&rotateAroundActiveNodeRadioBtn, static_cast<int>(RotationCenter::ActiveNode));
    rotationCenterGroup.addButton(&rotateAroundDatasetCenterRadioBtn, static_cast<int>(RotationCenter::DatasetCenter));
    rotationCenterGroup.addButton(&rotateAroundCurrentPositionRadioBtn, static_cast<int>(RotationCenter::CurrentPosition));
    generalLayout.addWidget(&showScalebarCheckBox);
    generalLayout.addWidget(&showVPDecorationCheckBox);

    crosshairDropdown.addItems({tr("Subtle"), tr("Strong"), tr("Hidden")});
    crosshairLayout.addWidget(&crosshairLabel);
    crosshairLayout.addWidget(&crosshairDropdown);

    generalLayout.addLayout(&crosshairLayout);
    generalLayout.addWidget(&addArbVPCheckBox);
    generalLayout.setAlignment(Qt::AlignTop);
    generalBox.setLayout(&generalLayout);

    planesLayout.addWidget(&showXYPlaneCheckBox);
    planesLayout.addWidget(&showXZPlaneCheckBox);
    planesLayout.addWidget(&showZYPlaneCheckBox);
    planesLayout.addWidget(&showArbPlaneCheckBox);
    planesBox.setCheckable(true);
    planesBox.setLayout(&planesLayout);
    boundaryLayout.addWidget(&boundariesPixelRadioBtn);
    boundaryLayout.addWidget(&boundariesPhysicalRadioBtn);
    boundaryBox.setLayout(&boundaryLayout);
    rotationLayout.addWidget(&rotateAroundActiveNodeRadioBtn);
    rotationLayout.addWidget(&rotateAroundDatasetCenterRadioBtn);
    rotationLayout.addWidget(&rotateAroundCurrentPositionRadioBtn);
    rotationBox.setLayout(&rotationLayout);

    viewport3DLayout.addWidget(&planesBox);
    viewport3DLayout.addWidget(&boundaryBox);
    viewport3DLayout.addWidget(&rotationBox);
    viewport3DLayout.setAlignment(Qt::AlignTop);
    viewport3DBox.setLayout(&viewport3DLayout);

    mainLayout.addWidget(&generalBox);
    mainLayout.addWidget(&viewport3DBox);
    setLayout(&mainLayout);

    QObject::connect(&showScalebarCheckBox, &QCheckBox::stateChanged, [] (int checkState) { state->viewerState->showScalebar = checkState; });
    QObject::connect(&showVPDecorationCheckBox, &QCheckBox::clicked, this, [] (const bool checked) {
        state->viewerState->showVpDecorations = checked;
        state->viewer->mainWindow.forEachVPDo([&checked](ViewportBase & vp) { vp.showHideButtons(checked); });
    });
    QObject::connect(&crosshairDropdown, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [](const int index) {
        state->viewerState->crosshairDisplay = static_cast<CrosshairDisplay>(index);
    });
    QObject::connect(&addArbVPCheckBox, &QCheckBox::clicked, [this](const bool on) {
        showArbPlaneCheckBox.setEnabled(on);
        // HACK register arb box to group (doesn’t toggle otherwise)
        planesBox.setChecked(!planesBox.isChecked());
        planesBox.setChecked(!planesBox.isChecked());
        state->viewerState->enableArbVP = on;
        state->viewer->viewportArb->setVisible(on);
    });
    QObject::connect(state->viewer, &Viewer::enabledArbVP, [this] (const bool on) {
        addArbVPCheckBox.setChecked(on);
        addArbVPCheckBox.clicked(on);
    });

    // 3D viewport
    QObject::connect(&planesBox, &QGroupBox::clicked, [](bool checked) { state->viewerState->showVpPlanes = checked; });
    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXYplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXZplane = checked; });
    QObject::connect(&showZYPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showZYplane = checked; });
    QObject::connect(&showArbPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showArbplane = checked; });
    QObject::connect(&boundaryGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), [this](const QAbstractButton *, bool) {
        Viewport3D::showBoundariesInUm = boundariesPhysicalRadioBtn.isChecked();
    });
    QObject::connect(&rotationCenterGroup, &QButtonGroup::idToggled, [](const int id, const bool checked) {
        if (checked) {
            state->viewerState->rotationCenter = static_cast<RotationCenter>(id);
        }
    });
}

void ViewportTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_SCALEBAR, showScalebarCheckBox.checkState());
    settings.setValue(SHOW_VP_DECORATION, showVPDecorationCheckBox.isChecked());
    settings.setValue(CROSSHAIR_DISPLAY, crosshairDropdown.currentIndex());
    settings.setValue(ADD_ARB_VP, addArbVPCheckBox.isChecked());
    settings.setValue(SHOW_VP_PLANES, planesBox.isChecked());
    settings.setValue(SHOW_XY_PLANE, showXYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_XZ_PLANE, showXZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_ZY_PLANE, showZYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_ARB_PLANE, showArbPlaneCheckBox.isChecked());
    settings.setValue(SHOW_PHYSICAL_BOUNDARIES, boundariesPhysicalRadioBtn.isChecked());
    settings.setValue(ROTATION_CENTER, rotationCenterGroup.checkedId());
}

void ViewportTab::loadSettings(const QSettings & settings) {
    bool ok{false};
    int scaleBarSetting = settings.value(SHOW_SCALEBAR, 0).toInt(&ok);
    if (!ok) {
        scaleBarSetting = settings.value(SHOW_SCALEBAR, false).toBool();
    }
    showScalebarCheckBox.setCheckState(static_cast<Qt::CheckState>(scaleBarSetting));
    showVPDecorationCheckBox.setChecked(settings.value(SHOW_VP_DECORATION, true).toBool());
    showVPDecorationCheckBox.clicked(showVPDecorationCheckBox.isChecked());
    crosshairDropdown.setCurrentIndex(settings.value(CROSSHAIR_DISPLAY, static_cast<int>(devicePixelRatio() > 1 ? CrosshairDisplay::STRONG : CrosshairDisplay::SUBTLE)).toInt());
    addArbVPCheckBox.setChecked(settings.value(ADD_ARB_VP, false).toBool());
    addArbVPCheckBox.clicked(addArbVPCheckBox.isChecked());
    planesBox.setChecked(settings.value(SHOW_VP_PLANES, true).toBool());
    planesBox.clicked(planesBox.isChecked());
    showXYPlaneCheckBox.setChecked(settings.value(SHOW_XY_PLANE, true).toBool());
    showXYPlaneCheckBox.clicked(showXYPlaneCheckBox.isChecked());
    showXZPlaneCheckBox.setChecked(settings.value(SHOW_XZ_PLANE, true).toBool());
    showXZPlaneCheckBox.clicked(showXZPlaneCheckBox.isChecked());
    showZYPlaneCheckBox.setChecked(settings.value(SHOW_ZY_PLANE, true).toBool());
    showZYPlaneCheckBox.clicked(showZYPlaneCheckBox.isChecked());
    showArbPlaneCheckBox.setChecked(settings.value(SHOW_ARB_PLANE, true).toBool());
    showArbPlaneCheckBox.clicked(showArbPlaneCheckBox.isChecked());
    const auto showPhysicalBoundaries = settings.value(SHOW_PHYSICAL_BOUNDARIES, false).toBool();
    boundariesPixelRadioBtn.setChecked(!showPhysicalBoundaries);
    boundariesPhysicalRadioBtn.setChecked(showPhysicalBoundaries);
    boundaryGroup.buttonToggled(&boundariesPhysicalRadioBtn, boundariesPhysicalRadioBtn.isChecked());

    auto * rotationButton = rotationCenterGroup.button(settings.value(ROTATION_CENTER, rotationCenterGroup.id(&rotateAroundActiveNodeRadioBtn)).toInt());
    rotationButton->setChecked(true);
    rotationCenterGroup.buttonToggled(rotationButton, true);
}

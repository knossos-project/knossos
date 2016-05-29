#include "viewporttab.h"
#include "widgets/GuiConstants.h"
#include "viewer.h"
#include "widgets/viewport.h"

ViewportTab::ViewportTab(QWidget *parent) : QWidget(parent) {
    boundaryGroup.addButton(&boundariesPixelRadioBtn);
    boundaryGroup.addButton(&boundariesPhysicalRadioBtn);

    rotationCenterGroup.addButton(&rotateAroundDatasetCenterRadioBtn);
    rotationCenterGroup.addButton(&rotateAroundActiveNodeRadioBtn);
    rotationCenterGroup.addButton(&rotateAroundCurrentPositionRadioBtn);

    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);

    generalLayout.addWidget(&showScalebarCheckBox);
    generalLayout.addWidget(&showVPDecorationCheckBox);
    generalLayout.addWidget(&drawIntersectionsCrossHairCheckBox);

    viewport3DLayout.addWidget(&showXYPlaneCheckBox);
    viewport3DLayout.addWidget(&showXZPlaneCheckBox);
    viewport3DLayout.addWidget(&showZYPlaneCheckBox);
    viewport3DLayout.addWidget(&showArbPlaneCheckBox);
    viewport3DLayout.addWidget(&boundariesPixelRadioBtn);
    viewport3DLayout.addWidget(&boundariesPhysicalRadioBtn);
    viewport3DLayout.addWidget(&rotateAroundDatasetCenterRadioBtn);
    viewport3DLayout.addWidget(&rotateAroundActiveNodeRadioBtn);
    viewport3DLayout.addWidget(&rotateAroundCurrentPositionRadioBtn);
    int row = 0;
    mainLayout.setAlignment(Qt::AlignTop);
    mainLayout.addWidget(&generalHeader, row, 0); mainLayout.addWidget(&viewport3DHeader, row++, 1);
    mainLayout.addWidget(&separator, row++, 0, 1, 2);
    mainLayout.addLayout(&generalLayout, row, 0); mainLayout.addLayout(&viewport3DLayout, row++, 1);
    setLayout(&mainLayout);

    QObject::connect(&showScalebarCheckBox, &QCheckBox::clicked, [] (bool checked) { state->viewerState->showScalebar = checked; });
    QObject::connect(&showVPDecorationCheckBox, &QCheckBox::clicked, this, &ViewportTab::setViewportDecorations);
    QObject::connect(&drawIntersectionsCrossHairCheckBox, &QCheckBox::clicked, [](const bool on) { state->viewerState->drawVPCrosshairs = on; });
    // 3D viewport
    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXYplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXZplane = checked; });
    QObject::connect(&showZYPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showZYplane = checked; });
    QObject::connect(&showArbPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showArbplane = checked; });
    QObject::connect(&boundaryGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), [this](const QAbstractButton *, bool) {
        Viewport3D::showBoundariesInUm = boundariesPhysicalRadioBtn.isChecked();
    });
    QObject::connect(&rotationCenterGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), [this](const QAbstractButton *, bool) {
        state->viewerState->rotationCenter = (rotateAroundDatasetCenterRadioBtn.isChecked()) ? RotationCenter::DatasetCenter :
                                             (rotateAroundActiveNodeRadioBtn.isChecked()) ? RotationCenter::ActiveNode :
                                             RotationCenter::CurrentPosition;
    });
}

void ViewportTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_SCALEBAR, showScalebarCheckBox.isChecked());
    settings.setValue(SHOW_VP_DECORATION, showVPDecorationCheckBox.isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, drawIntersectionsCrossHairCheckBox.isChecked());
    settings.setValue(SHOW_XY_PLANE, showXYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_XZ_PLANE, showXZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_ZY_PLANE, showZYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_ARB_PLANE, showArbPlaneCheckBox.isChecked());
    settings.setValue(SHOW_PHYSICAL_BOUNDARIES, boundariesPhysicalRadioBtn.isChecked());
    settings.setValue(ROTATION_CENTER, rotationCenterGroup.checkedId());
}

void ViewportTab::loadSettings(const QSettings & settings) {
    showScalebarCheckBox.setChecked(settings.value(SHOW_SCALEBAR, false).toBool());
    showScalebarCheckBox.clicked(showScalebarCheckBox.isChecked());
    showVPDecorationCheckBox.setChecked(settings.value(SHOW_VP_DECORATION, true).toBool());
    showVPDecorationCheckBox.clicked(showVPDecorationCheckBox.isChecked());
    drawIntersectionsCrossHairCheckBox.setChecked(settings.value(DRAW_INTERSECTIONS_CROSSHAIRS, true).toBool());
    drawIntersectionsCrossHairCheckBox.clicked(drawIntersectionsCrossHairCheckBox.isChecked());
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

    auto * rotationButton = rotationCenterGroup.button(settings.value(ROTATION_CENTER, rotationCenterGroup.id(&rotateAroundDatasetCenterRadioBtn)).toInt());
    rotationButton->setChecked(true);
    rotationCenterGroup.buttonToggled(rotationButton, true);
}

#include "viewporttab.h"
#include "widgets/GuiConstants.h"
#include "viewer.h"
#include "widgets/viewport.h"

ViewportTab::ViewportTab(QWidget *parent) : QWidget(parent)
{
    boundaryGroup.addButton(&boundariesPixelRadioBtn);
    boundaryGroup.addButton(&boundariesPhysicalRadioBtn);

    resetVPsButton.setFocusPolicy(Qt::NoFocus);

    separator.setFrameShape(QFrame::HLine);
    separator.setFrameShadow(QFrame::Sunken);

    generalLayout.addWidget(&showScalebarCheckBox);
    generalLayout.addWidget(&showVPDecorationCheckBox);
    generalLayout.addWidget(&drawIntersectionsCrossHairCheckBox);
    generalLayout.addWidget(&arbitraryModeCheckBox);

    viewport3DLayout.addWidget(&showXYPlaneCheckBox);
    viewport3DLayout.addWidget(&showXZPlaneCheckBox);
    viewport3DLayout.addWidget(&showYZPlaneCheckBox);
    viewport3DLayout.addWidget(&boundariesPixelRadioBtn);
    viewport3DLayout.addWidget(&boundariesPhysicalRadioBtn);
    viewport3DLayout.addWidget(&rotateAroundActiveNodeCheckBox);
    int row = 0;
    mainLayout.setAlignment(Qt::AlignTop);
    mainLayout.addWidget(&generalHeader, row, 0); mainLayout.addWidget(&viewport3DHeader, row++, 1);
    mainLayout.addWidget(&separator, row++, 0, 1, 2);
    mainLayout.addLayout(&generalLayout, row, 0); mainLayout.addLayout(&viewport3DLayout, row++, 1);
    mainLayout.addWidget(&resetVPsButton, row++, 0, Qt::AlignBottom | Qt::AlignLeft);
    setLayout(&mainLayout);

    QObject::connect(&showScalebarCheckBox, &QCheckBox::clicked, [] (bool checked) { state->viewerState->showScalebar = checked; });
    QObject::connect(&showVPDecorationCheckBox, &QCheckBox::clicked, this, &ViewportTab::setViewportDecorations);
    QObject::connect(&drawIntersectionsCrossHairCheckBox, &QCheckBox::clicked, [](const bool on) { state->viewerState->drawVPCrosshairs = on; });
    QObject::connect(&arbitraryModeCheckBox, &QCheckBox::clicked, [&](bool checked) {
        Viewport::arbitraryOrientation = checked;
        emit setVPOrientationSignal(checked);
    });
    // 3D viewport
    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXYplane = checked; });
    QObject::connect(&showYZPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showYZplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::clicked, [](bool checked) { state->viewerState->showXZplane = checked; });
    QObject::connect(&boundaryGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled), [this](const QAbstractButton *, bool) {
        Viewport::showBoundariesInUm = boundariesPhysicalRadioBtn.isChecked();
    });
    QObject::connect(&rotateAroundActiveNodeCheckBox, &QCheckBox::clicked, [](bool checked) {state->viewerState->rotateAroundActiveNode = checked; });

    QObject::connect(&resetVPsButton, &QPushButton::clicked, this, &ViewportTab::resetViewportPositions);
}

void ViewportTab::saveSettings(QSettings & settings) const {
    settings.setValue(SHOW_SCALEBAR, showScalebarCheckBox.isChecked());
    settings.setValue(SHOW_VP_DECORATION, showVPDecorationCheckBox.isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, drawIntersectionsCrossHairCheckBox.isChecked());
    settings.setValue(SHOW_XY_PLANE, showXYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_XZ_PLANE, showXZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_YZ_PLANE, showYZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_PHYSICAL_BOUNDARIES, boundariesPhysicalRadioBtn.isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, rotateAroundActiveNodeCheckBox.isChecked());
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
    showYZPlaneCheckBox.setChecked(settings.value(SHOW_YZ_PLANE, true).toBool());
    showYZPlaneCheckBox.clicked(showYZPlaneCheckBox.isChecked());
    const auto showPhysicalBoundaries = settings.value(SHOW_PHYSICAL_BOUNDARIES, false).toBool();
    boundariesPixelRadioBtn.setChecked(!showPhysicalBoundaries);
    boundariesPhysicalRadioBtn.setChecked(showPhysicalBoundaries);
    boundaryGroup.buttonToggled(&boundariesPhysicalRadioBtn, boundariesPhysicalRadioBtn.isChecked());
    rotateAroundActiveNodeCheckBox.setChecked(settings.value(ROTATE_AROUND_ACTIVE_NODE, true).toBool());
    rotateAroundActiveNodeCheckBox.clicked(rotateAroundActiveNodeCheckBox.isChecked());
}

#include "viewporttab.h"

#include "skeleton/skeletonizer.h"
#include "viewer.h"
#include "../viewport.h"

ViewportTab::ViewportTab(QWidget *parent) : QWidget(parent)
{
    auto boundaryGroup = new QButtonGroup(this);
    boundaryGroup->addButton(&boundariesPixelRadioBtn);
    boundaryGroup->addButton(&boundariesPhysicalRadioBtn);

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

    QObject::connect(&showScalebarCheckBox, &QCheckBox::toggled, [] (bool checked) { state->viewerState->showScalebar = checked; });
    QObject::connect(&showVPDecorationCheckBox, &QCheckBox::toggled, this, &ViewportTab::setViewportDecorations);
    QObject::connect(&drawIntersectionsCrossHairCheckBox, &QCheckBox::toggled, [](const bool on) { state->viewerState->drawVPCrosshairs = on; });
    QObject::connect(&arbitraryModeCheckBox, &QCheckBox::toggled, [&](bool checked) {
        Viewport::arbitraryOrientation = checked;
        emit setVPOrientationSignal(checked);
    });
    // 3D viewport
    QObject::connect(&showXYPlaneCheckBox, &QCheckBox::toggled, [](bool checked) { state->skeletonState->showXYplane = checked; });
    QObject::connect(&showYZPlaneCheckBox, &QCheckBox::toggled, [](bool checked) { state->skeletonState->showYZplane = checked; });
    QObject::connect(&showXZPlaneCheckBox, &QCheckBox::toggled, [](bool checked) { state->skeletonState->showXZplane = checked; });
    QObject::connect(boundaryGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), [this](const int, const bool) {
        Viewport::showBoundariesInUm = boundariesPhysicalRadioBtn.isChecked();
    });
    QObject::connect(&rotateAroundActiveNodeCheckBox, &QCheckBox::toggled, [](bool checked) {state->skeletonState->rotateAroundActiveNode = checked; });

    QObject::connect(&resetVPsButton, &QPushButton::clicked, this, &ViewportTab::resetViewportPositions);
}

#include "vpskeletonviewportwidget.h"
#include <QLabel>
#include <QFrame>
#include <QCheckBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include "knossos-global.h"
extern struct stateInfo *state;

VPSkeletonViewportWidget::VPSkeletonViewportWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout = new QGridLayout();

    datasetVisualizationLabel = new QLabel("Dataset Visualization");
    showXYPlaneCheckBox = new QCheckBox("Show XY Plane");
    showXZPlaneCheckBox = new QCheckBox("Show XZ Plane");
    showYZPlaneCheckBox = new QCheckBox("Show YZ Plane");

    skeletonDisplayModesLabel = new QLabel("Skeleton Display Modes");
    wholeSkeletonButton = new QRadioButton("Whole Skeleton");
    onlyCurrentCubeButton = new QRadioButton("Only Current Cube");
    onlyActiveTreeButton = new QRadioButton("Only Active Tree");
    hideSkeletonButton = new QRadioButton("Hide Skeleton (fast)");

    view3dlabel = new QLabel("View 3D");
    rotateAroundActiveNodeCheckBox = new QCheckBox("Rotate Around Active Node");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    gridLayout->addWidget(datasetVisualizationLabel, 0, 0);
    gridLayout->addWidget(skeletonDisplayModesLabel, 0, 1);
    gridLayout->addWidget(line, 1, 0);
    gridLayout->addWidget(line2, 1, 1);
    gridLayout->addWidget(showXYPlaneCheckBox, 2, 0);
    gridLayout->addWidget(wholeSkeletonButton, 2, 1);
    gridLayout->addWidget(showXZPlaneCheckBox, 3, 0);
    gridLayout->addWidget(onlyCurrentCubeButton, 3, 1);
    gridLayout->addWidget(showYZPlaneCheckBox, 4, 0);
    gridLayout->addWidget(onlyActiveTreeButton, 4, 1);
    gridLayout->addWidget(hideSkeletonButton, 5, 1);

    gridLayout->addWidget(view3dlabel, 7, 0);
    gridLayout->addWidget(line3, 8, 0);
    gridLayout->addWidget(rotateAroundActiveNodeCheckBox, 9, 0);


    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
    mainLayout->addStretch(50);

    connect(showXYPlaneCheckBox, SIGNAL(clicked(bool)), this, SLOT(showXYPlaneChecked(bool)));
    connect(showYZPlaneCheckBox, SIGNAL(clicked(bool)), this, SLOT(showYZPlaneChecked(bool)));
    connect(showXZPlaneCheckBox, SIGNAL(clicked(bool)), this, SLOT(showXZPlaneChecked(bool)));
    connect(wholeSkeletonButton, SIGNAL(clicked()), this, SLOT(wholeSkeletonSelected()));
    connect(onlyCurrentCubeButton, SIGNAL(clicked()), this, SLOT(onlyCurrentCubeSelected()));
    connect(onlyActiveTreeButton, SIGNAL(clicked()), this, SLOT(onlyActiveTreeSelected()));
    connect(hideSkeletonButton, SIGNAL(clicked()), this, SLOT(hideSkeletonSelected()));
    connect(rotateAroundActiveNodeCheckBox, SIGNAL(clicked(bool)), SLOT(rotateAroundActiveNodeChecked(bool)));
}

void VPSkeletonViewportWidget::loadSettings() {
    showXYPlaneCheckBox->setChecked(state->skeletonState->showXYplane);
    showYZPlaneCheckBox->setChecked(state->skeletonState->showYZplane);
    showXZPlaneCheckBox->setChecked(state->skeletonState->showXZplane);

    rotateAroundActiveNodeCheckBox->setChecked(state->skeletonState->rotateAroundActiveNode);
}

void VPSkeletonViewportWidget::showXYPlaneChecked(bool on) {
    state->skeletonState->showXYplane = on;
}

void VPSkeletonViewportWidget::showXZPlaneChecked(bool on) {
    state->skeletonState->showXZplane = on;
}

void VPSkeletonViewportWidget::showYZPlaneChecked(bool on) {
    state->skeletonState->showYZplane = on;
}

/**
  * @todo numeric order of radio buttons
  */
void VPSkeletonViewportWidget::wholeSkeletonSelected() {

}

void VPSkeletonViewportWidget::onlyCurrentCubeSelected() {

}

void VPSkeletonViewportWidget::onlyActiveTreeSelected() {

}

void VPSkeletonViewportWidget::hideSkeletonSelected() {

}

void VPSkeletonViewportWidget::rotateAroundActiveNodeChecked(bool on) {
    state->skeletonState->rotateAroundActiveNode = on;
}


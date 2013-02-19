#include "vpgeneraltabwidget.h"
#include <QLabel>
#include <QSpinBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include "knossos-global.h"
extern struct stateInfo *state;

VPGeneralTabWidget::VPGeneralTabWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    this->skeletonVisualizationLabel = new QLabel("Skeleton Visualization");
    this->skeletonRenderingModelLabel = new QLabel("Skeleton Rendering Model");

    this->lightEffectsButton = new QCheckBox("Light Effects");
    this->lightEffectsButton->setChecked(state->viewerState->lightOnOff);

    this->hightlightActiveTreeButton = new QCheckBox("Hilight Active Tree");
    this->hightlightActiveTreeButton->setChecked(state->viewerState->highlightVp);

    this->showAllNodeIdsButton = new QCheckBox("Show All Node IDs");
    this->showAllNodeIdsButton->setChecked(state->viewerState->showVPLabels);

    this->overrideNodeRadiusButton = new QCheckBox("Override Node Radius");
    this->overrideNodeRadiusButton->setChecked(state->skeletonState->overrideNodeRadiusBool);



    this->edgeNodeRadiusRatioSpinBox = new QDoubleSpinBox();
    this->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);
    this->edgeNodeRadiusRatioLabel = new QLabel("Edge <-> Node Radius Ratio:");

    this->linesAndPointsButton = new QRadioButton("Lines and Points(fast)");

    this->Skeleton3dButton = new QRadioButton("3D Skeleton");

    if(!state->skeletonState->overrideNodeRadiusBool) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(false);
    }

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(skeletonVisualizationLabel, 1, 1);
    gridLayout->addWidget(skeletonRenderingModelLabel, 1, 2);
    gridLayout->addWidget(line, 2, 1);
    gridLayout->addWidget(line2, 2, 2);

    gridLayout->addWidget(this->lightEffectsButton, 3, 1);
    gridLayout->addWidget(linesAndPointsButton, 3, 2);
    gridLayout->addWidget(hightlightActiveTreeButton, 4, 1);
    gridLayout->addWidget(Skeleton3dButton, 4, 2);

    gridLayout->addWidget(showAllNodeIdsButton, 5, 1);
    gridLayout->addWidget(overrideNodeRadiusButton, 6, 1);
    gridLayout->addWidget(edgeNodeRadiusRatioLabel, 7, 1);
    gridLayout->addWidget(edgeNodeRadiusRatioSpinBox, 7, 2);


    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
    mainLayout->addStretch(50);

    connect(lightEffectsButton, SIGNAL(clicked(bool)), this, SLOT(lightEffectsChecked(bool)));
    connect(showAllNodeIdsButton, SIGNAL(clicked(bool)), this, SLOT(showAllNodeIdsChecked(bool)));
    connect(hightlightActiveTreeButton, SIGNAL(clicked(bool)), this, SLOT(hightlightActiveTreeChecked(bool)));
    connect(overrideNodeRadiusButton, SIGNAL(clicked(bool)), this, SLOT(overrideNodeRadiusChecked(bool)));
    connect(edgeNodeRadiusRatioSpinBox, SIGNAL(valueChanged(double)), this, SLOT(edgeNodeRadiusRatioChanged(double)));
    connect(linesAndPointsButton, SIGNAL(clicked(bool)), this, SLOT(linesAndPointsChecked(bool)));
    connect(Skeleton3dButton, SIGNAL(clicked(bool)), this, SLOT(Skeleton3dChecked(bool)));
}

void VPGeneralTabWidget::lightEffectsChecked(bool on) {
    state->viewerState->lightOnOff = on;
}

/**
  * @todo bool param is not needed here
  */
void VPGeneralTabWidget::hightlightActiveTreeChecked(bool on) {
    if(state->skeletonState->highlightActiveTree) {
        state->skeletonState->highlightActiveTree = false;
    } else {
        state->skeletonState->highlightActiveTree = true;
    }

    state->skeletonState->skeletonChanged = true;
}

/**
  * @todo bool param is not need here
  */
void VPGeneralTabWidget::showAllNodeIdsChecked(bool on) {
    if(state->skeletonState->showNodeIDs) {
        state->skeletonState->showNodeIDs = false;
    } else {
        state->skeletonState->showNodeIDs = true;
    }

    state->skeletonState->skeletonChanged = true;
}

void VPGeneralTabWidget::overrideNodeRadiusChecked(bool on) {
    state->skeletonState->overrideNodeRadiusBool = on;

    if(on) {
        this->edgeNodeRadiusRatioSpinBox->setEnabled(true);
    } else {
        this->edgeNodeRadiusRatioLabel->setEnabled(false);
    }
}

/**
  * @todo for what is overrideNodeRadiusVal ?
  */
void VPGeneralTabWidget::edgeNodeRadiusRatioChanged(double value) {
    state->skeletonState->segRadiusToNodeRadius = value;
}

/**
  * @todo the numeric order of radioRenderingModel
  */
void VPGeneralTabWidget::linesAndPointsChecked(bool on) {
    state->viewerState->gui->radioRenderingModel;
}

/**
  * @todo the numeric order of radioRanderingModel
  */
void VPGeneralTabWidget::Skeleton3dChecked(bool on) {
    state->viewerState->gui->radioRenderingModel;
}

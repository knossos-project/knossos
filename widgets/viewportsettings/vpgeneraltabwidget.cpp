#include "vpgeneraltabwidget.h"

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

    this->overrideNodeRadiusSpinBox = new QSpinBox();
    this->overrideNodeRadiusSpinBox->setValue(state->skeletonState->overrideNodeRadiusVal);

    this->edgeNodeRadiusRatioSpinBox = new QSpinBox();
    this->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);
    this->edgeNodeRadiusRatioLabel = new QLabel("Edge <-> Node Radius Ratio:");

    this->linesAndPointsButton = new QRadioButton("Lines and Points(fast)");

    this->Skeleton3dButton = new QRadioButton("3D Skeleton");

    if(!state->skeletonState->overrideNodeRadiusBool) {
        this->overrideNodeRadiusSpinBox->setEnabled(false);
        this->edgeNodeRadiusRatioSpinBox->setEnabled(false);
    }

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(skeletonVisualizationLabel, 0, 0);
    gridLayout->addWidget(skeletonRenderingModelLabel, 0, 2);
    gridLayout->addWidget(line, 1, 0);
    gridLayout->addWidget(line2, 1, 2);
    gridLayout->addWidget(this->lightEffectsButton, 2, 0);
    gridLayout->addWidget(linesAndPointsButton, 2, 2);
    gridLayout->addWidget(hightlightActiveTreeButton, 3, 0);
    gridLayout->addWidget(Skeleton3dButton, 3, 2);
    gridLayout->addWidget(showAllNodeIdsButton, 4, 0);
    gridLayout->addWidget(overrideNodeRadiusButton, 5, 0);
    gridLayout->addWidget(overrideNodeRadiusSpinBox, 5, 1);
    gridLayout->addWidget(edgeNodeRadiusRatioLabel, 6, 0);
    gridLayout->addWidget(edgeNodeRadiusRatioSpinBox, 6, 1);



    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);

    connect(lightEffectsButton, SIGNAL(clicked(bool)), this, SLOT(lightEffectsChecked(bool)));
    connect(showAllNodeIdsButton, SIGNAL(clicked(bool)), this, SLOT(showAllNodeIdsChecked(bool)));
    connect(hightlightActiveTreeButton, SIGNAL(clicked(bool)), this, SLOT(hightlightActiveTreeChecked(bool)));
    connect(overrideNodeRadiusButton, SIGNAL(clicked(bool)), this, SLOT(overrideNodeRadiusChecked(bool)));
    connect(linesAndPointsButton, SIGNAL(clicked(bool)), this, SLOT(linesAndPointsChecked(bool)));
    connect(Skeleton3dButton, SIGNAL(clicked(bool)), this, SLOT(Skeleton3dChecked(bool)));
}

void VPGeneralTabWidget::lightEffectsChecked(bool on) {
    if(on) {

    } else {

    }
}

void VPGeneralTabWidget::hightlightActiveTreeChecked(bool on) {
    if(on) {

    } else {

    }
}

void VPGeneralTabWidget::showAllNodeIdsChecked(bool on) {
    if(on) {

    } else {

    }
}

void VPGeneralTabWidget::overrideNodeRadiusChecked(bool on) {
    if(on) {
        this->overrideNodeRadiusSpinBox->setEnabled(true);
        this->edgeNodeRadiusRatioSpinBox->setEnabled(true);
    } else {
        this->overrideNodeRadiusSpinBox->setEnabled(false);
        this->edgeNodeRadiusRatioLabel->setEnabled(false);
    }
}

void VPGeneralTabWidget::linesAndPointsChecked(bool on) {
    if(on) {

    } else {

    }
}

void VPGeneralTabWidget::Skeleton3dChecked(bool on) {
    if(on) {

    } else {

    }
}

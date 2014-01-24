#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "skeletonizer.h"
#include "toolscommandstab.h"

extern stateInfo *state;

ToolsCommandsTab::ToolsCommandsTab(QWidget *parent) :
    QWidget(parent)
{
    treeLabel = new QLabel("Tree");
    activeTreeLabel = new QLabel("Active Tree: 0");
    activeNodeLabel = new QLabel("Active Node: 0");
    activeTreeIDLabel = new QLabel("Active Tree ID:");
    activeTreeIDSpin = new QSpinBox();
    activeTreeIDSpin->setMaximum(state->skeletonState->greatestTreeID);
    activeNodeIDLabel = new QLabel("Active Node ID:");
    activeNodeIDSpin = new QSpinBox();
    activeNodeIDSpin->setMaximum(state->skeletonState->greatestNodeID);
    newTreeButton = new QPushButton("New Tree (C)");

    newTreeButton->setToolTip("Create a new tree");
    jumpToActiveButton = new QPushButton("Jump To Active Node (S)");
    jumpToActiveButton->setToolTip("Recenter on active node");

    branchnodesLabel = new QLabel("Branch Nodes");
    branchesOnStackLabel = new QLabel("On Stack: 0");
    pushBranchButton = new QPushButton("Push (B)ranch Node");
    pushBranchButton->setToolTip("Turn active node into a branch node");
    popBranchButton = new QPushButton("Pop && (J)ump");
    popBranchButton->setToolTip("Pop last branch node and jump to it");

    radiusLabel = new QLabel("Node Radius");
    useLastRadiusAsDefaultCheck = new QCheckBox("Use Last Radius As Default");
    defaultRadiusLabel = new QLabel("Default Node Radius:");
    defaultRadiusSpin = new QDoubleSpinBox();
    defaultRadiusSpin->setValue(state->skeletonState->defaultNodeRadius);
    defaultRadiusSpin->setMaximum(100000);
    defaultRadiusSpin->setMinimum(0.01);

    lockingLabel = new QLabel("Locking");
    commentLockingCheck = new QCheckBox("Enable Comment Locking");
    commentLockingCheck->setToolTip("Enable locking to nodes by comment");
    lockingRadiusLabel = new QLabel("Locking Radius:");
    lockingRadiusLabel->setToolTip("Set tracing radius around locked node");
    lockingRadiusSpin = new QSpinBox();
    lockingRadiusSpin->setToolTip("Set tracing radius around locked node");
    lockingRadiusSpin->setMaximum(100000);
    lockToNodesLabel = new QLabel("Lock To Nodes With Comment:");
    lockToNodesLabel->setToolTip("Needs comment locking enabled");
    commentLockEdit = new QLineEdit("seed");
    commentLockEdit->setToolTip("Needs comment locking enabled");
    locktoActiveButton = new QPushButton("Lock To Active Node");
    locktoActiveButton->setToolTip("Lock tracing to active node");
    disableCurrentLockButton = new QPushButton("Disable Current Locking");
    disableCurrentLockButton->setToolTip("Disable the currently active lock");

    mainLayout = new QVBoxLayout();
    QHBoxLayout *hLayout;
    QFrame *line;

    mainLayout->addWidget(treeLabel);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(activeTreeLabel);
    hLayout->addWidget(activeNodeLabel);
    mainLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(activeTreeIDLabel);
    hLayout->addWidget(activeTreeIDSpin);
    hLayout->addWidget(activeNodeIDLabel);
    hLayout->addWidget(activeNodeIDSpin);
    mainLayout->addLayout(hLayout);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(newTreeButton);
    hLayout->addWidget(jumpToActiveButton);
    mainLayout->addLayout(hLayout);

    mainLayout->addWidget(branchnodesLabel);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    mainLayout->addWidget(branchesOnStackLabel);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(pushBranchButton);
    hLayout->addWidget(popBranchButton);
    mainLayout->addLayout(hLayout);

    mainLayout->addWidget(radiusLabel);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    mainLayout->addWidget(useLastRadiusAsDefaultCheck);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(defaultRadiusLabel);
    hLayout->addWidget(defaultRadiusSpin);
    mainLayout->addLayout(hLayout);

    mainLayout->addWidget(lockingLabel);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    mainLayout->addWidget(commentLockingCheck);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(lockingRadiusLabel);
    hLayout->addWidget(lockingRadiusSpin);
    mainLayout->addLayout(hLayout);
    mainLayout->addWidget(lockToNodesLabel);
    mainLayout->addWidget(commentLockEdit);
    hLayout = new QHBoxLayout();
    hLayout->addWidget(locktoActiveButton);
    hLayout->addWidget(disableCurrentLockButton);
    mainLayout->addLayout(hLayout);

    setLayout(mainLayout);

    // widget event handling
    connect(activeTreeIDSpin, SIGNAL(valueChanged(int)), this, SLOT(activeTreeIDSpinChanged(int)));
    connect(activeNodeIDSpin, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIDSpinChanged(int)));
    connect(jumpToActiveButton, SIGNAL(clicked()), this, SLOT(jumpToActiveButtonClicked()));
    connect(newTreeButton, SIGNAL(clicked()), this, SLOT(newTreeButtonClicked()));
    connect(pushBranchButton, SIGNAL(clicked()), this, SLOT(pushBranchButtonClicked()));
    connect(popBranchButton, SIGNAL(clicked()), this, SLOT(popBranchButtonClicked()));
    connect(useLastRadiusAsDefaultCheck, SIGNAL(clicked(bool)), this, SLOT(useLastRadiusAsDefaultCheckChanged(bool)));
    connect(defaultRadiusSpin, SIGNAL(valueChanged(double)), this, SLOT(defaultRadiusSpinChanged(double)));
    connect(commentLockingCheck, SIGNAL(clicked(bool)), this, SLOT(commentLockingCheckChanged(bool)));
    connect(lockingRadiusSpin, SIGNAL(valueChanged(int)), this, SLOT(lockingRadiusSpinChanged(int)));
    connect(commentLockEdit, SIGNAL(textChanged(QString)), this, SLOT(commentLockEditChanged(QString)));
    connect(locktoActiveButton, SIGNAL(clicked()), this, SLOT(locktoActiveButtonClicked()));
    connect(disableCurrentLockButton, SIGNAL(clicked()), this, SLOT(disableCurrentLockButtonClicked()));
}

void ToolsCommandsTab::activeTreeIDSpinChanged(int value) {
    treeListElement *tree = findTreeByTreeIDSignal(value);
    if(tree == NULL) {
        return;
//        // no tree with this value.
//        // jump to next or previous tree, depending on wether spin box value is greater or smaller than active tree id
//        if(state->skeletonState->activeTree == NULL) { // no active node, activate first node found
//            tree = Skeletonizer::getTreeWithNextID(NULL);
//        }
//        else {
//            if(value > state->skeletonState->activeTree->treeID) { // move to next node
//                tree = Skeletonizer::getTreeWithNextID(state->skeletonState->activeTree);
//            }
//            else if(value < state->skeletonState->activeTree->treeID) {
//                tree = Skeletonizer::getTreeWithPrevID(state->skeletonState->activeTree);
//            }
//        }
    }
    if(tree) {
        setActiveTreeSignal(tree->treeID);
        emit treeActivatedSignal();
    }
    return;
}

void ToolsCommandsTab::activeNodeIDSpinChanged(int value) {
    nodeListElement *node = findNodeByNodeIDSignal(value);
    if(node == NULL) {
        return;
//        // no node with this value.
//        // jump to next or previous node, depending on wether spin box value is greater or smaller than active node id
//        if(state->skeletonState->activeNode == NULL) { // no active node, activate first node found
//            node = Skeletonizer::getNodeWithNextID(NULL, false);
//        }
//        else {
//            if((uint)value > state->skeletonState->activeNode->nodeID) { // move to next node
//                node = Skeletonizer::getNodeWithNextID(state->skeletonState->activeNode, false);
//            }
//            else if((uint)value < state->skeletonState->activeNode->nodeID) {
//                node = Skeletonizer::getNodeWithPrevID(state->skeletonState->activeNode, false);
//            }
//        }
    }
    if(node) {
        setActiveNodeSignal(CHANGE_MANUAL, node, 0);
        emit nodeActivatedSignal();
    }
    return;
}


void ToolsCommandsTab::jumpToActiveButtonClicked() {
    emit jumpToNodeSignal();
}

void ToolsCommandsTab::newTreeButtonClicked() {
    color4F treeCol;
    treeCol.r = -1;
    treeCol.g = -1;
    treeCol.b = -1;
    treeCol.a = 1;
    treeListElement *tree = addTreeListElement(true, CHANGE_MANUAL, 0, treeCol, true);
    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    emit treeAddedSignal(tree);
}

void ToolsCommandsTab::pushBranchButtonClicked() {
    if(pushBranchNodeSignal(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0, true)) {
        branchesOnStackLabel->setText(QString("On Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
        emit branchPushedSignal();
    }
}

void ToolsCommandsTab::popBranchButtonClicked() {
    if(popBranchNodeSignal()) {
        branchesOnStackLabel->setText(QString("On Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
        emit branchPoppedSignal();
    }
}

void ToolsCommandsTab::useLastRadiusAsDefaultCheckChanged(bool on) {
    if(on) {
        if(state->skeletonState->activeNode) {
            state->skeletonState->defaultNodeRadius = state->skeletonState->activeNode->radius;
        }
        return;
    }
    state->skeletonState->defaultNodeRadius = defaultRadiusSpin->value();
}

void ToolsCommandsTab::defaultRadiusSpinChanged(double value) {
    state->skeletonState->defaultNodeRadius = value;
}

void ToolsCommandsTab::commentLockingCheckChanged(bool on) {
    state->skeletonState->positionLocked = on;
    if(on and commentLockEdit->text().isEmpty() == false) {
        state->viewerState->gui->lockComment = commentLockEdit->text();
    }
}

void ToolsCommandsTab::lockingRadiusSpinChanged(int value) {
    state->skeletonState->lockRadius = value;
}

void ToolsCommandsTab::commentLockEditChanged(QString comment) {
    state->viewerState->gui->lockComment = comment;
}

void ToolsCommandsTab::locktoActiveButtonClicked() {
    if(state->skeletonState->activeNode) {
        emit lockPositionSignal(state->skeletonState->activeNode->position);
    }
    else {
        qDebug("There is not active node to lock");
    }
}

void ToolsCommandsTab::disableCurrentLockButtonClicked() {
    emit unlockPositionSignal();
}

void ToolsCommandsTab::update() {
    activeTreeIDSpin->setMaximum(state->skeletonState->greatestTreeID);
    activeNodeIDSpin->setMaximum(state->skeletonState->greatestNodeID);
    if(state->skeletonState->activeTree) {
        activeTreeLabel->setText(QString("Active Tree: %1").arg(state->skeletonState->activeTree->treeID));
        if(activeTreeIDSpin->value() != state->skeletonState->activeTree->treeID) {
            activeTreeIDSpin->setValue(state->skeletonState->activeTree->treeID);
        }
    }
    else {
        activeTreeLabel->setText("Active Tree: 0");
        activeTreeIDSpin->setValue(0);
    }

    if(state->skeletonState->activeNode) {
        activeNodeLabel->setText(QString("Active Node: %1").arg(state->skeletonState->activeNode->nodeID));
        if(activeNodeIDSpin->value() != state->skeletonState->activeNode->nodeID) {
            activeNodeIDSpin->setValue(state->skeletonState->activeNode->nodeID);
        }
    }
    else {
        activeNodeLabel->setText("Active Node: 0");
        activeNodeIDSpin->setValue(0);
    }
    defaultRadiusSpin->setValue(state->skeletonState->defaultNodeRadius);
    branchesOnStackLabel->setText(QString("On Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}

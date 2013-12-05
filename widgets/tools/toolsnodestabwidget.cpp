/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2013
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
 */

/*
 * For further information, visit http://www.knossostool.org or contact
 *     Joergen.Kornfeld@mpimf-heidelberg.mpg.de or
 *     Fabian.Svara@mpimf-heidelberg.mpg.de
 */

#include <QLabel>
#include <QSpinBox>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>

#include "knossos.h"
#include "skeletonizer.h"
#include "toolsnodestabwidget.h"

extern struct stateInfo *state;

ToolsNodesTabWidget::ToolsNodesTabWidget(ToolsWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    activeNodeIdLabel = new QLabel("Active Node ID");
    activeNodeIdSpinBox = new QSpinBox();
    activeNodeIdSpinBox->setMinimum(0);

    activeNodeXLabel = new QLabel("x:");
    activeNodeYLabel = new QLabel("y:");
    activeNodeZLabel = new QLabel("z:");
    activeNodeXSpin = new QSpinBox();
    activeNodeYSpin = new QSpinBox();
    activeNodeZSpin = new QSpinBox();
    activeNodeXSpin->setMinimum(0);
    activeNodeYSpin->setMinimum(0);
    activeNodeZSpin->setMinimum(0);
    activeNodeXSpin->setMaximum(state->boundary.x);
    activeNodeYSpin->setMaximum(state->boundary.y);
    activeNodeZSpin->setMaximum(state->boundary.z);

    jumpToNodeButton = new QPushButton("Jump to node (S)");
    deleteNodeButton = new QPushButton("Delete Node (Del)");
    linkNodeWithButton = new QPushButton("Link Node with(Shift + Click)");

    idLabel = new QLabel("ID:");
    idSpinBox = new QSpinBox();
    idSpinBox->setMaximum(0); // no nodes on start-up
    idSpinBox->setMinimum(0);

    commentLabel = new QLabel("Comment:");
    searchForLabel = new QLabel("Search For:");
    commentField = new QLineEdit();
    searchForField = new QLineEdit();

    findNextButton = new QPushButton("Find (n)ext");
    findPreviousButton = new QPushButton("Find (p)revious");

    useLastRadiusBox = new QCheckBox("Use Last Radius as Default");
    activeNodeRadiusLabel = new QLabel("Active Node Radius(SHIFT + wheel):");
    activeNodeRadiusSpinBox = new QDoubleSpinBox();
    activeNodeRadiusSpinBox->setMaximum(100000);
    defaultNodeRadiusLabel = new QLabel("Default Node Radius:");
    defaultNodeRadiusSpinBox = new QDoubleSpinBox();
    defaultNodeRadiusSpinBox->setMaximum(100000);

    enableCommentLockingCheckBox = new QCheckBox("Enable comment locking");
    lockingRadiusLabel = new QLabel("Locking Radius:");
    lockToNodesWithCommentLabel = new QLabel("Lock To Nodes With Comment:");
    lockingRadiusSpinBox = new QSpinBox();
    lockingRadiusSpinBox->setMaximum(100000);
    lockingToNodesWithCommentField = new QLineEdit("seed");
    lockToActiveNodeButton = new QPushButton("Lock to Active Node");
    disableLockingButton = new QPushButton("Disable Locking");

    QFormLayout *formLayout;
    QGridLayout *gridLayout;
    QFrame *line;

    formLayout = new QFormLayout();
    formLayout->addRow(activeNodeIdLabel, activeNodeIdSpinBox);
    mainLayout->addLayout(formLayout);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(activeNodeXLabel, 1, 1);
    gridLayout->addWidget(activeNodeXSpin, 1, 2);
    gridLayout->addWidget(activeNodeYLabel, 1, 3);
    gridLayout->addWidget(activeNodeYSpin, 1, 4);
    gridLayout->addWidget(activeNodeZLabel, 1, 5);
    gridLayout->addWidget(activeNodeZSpin, 1, 6);
    mainLayout->addLayout(gridLayout);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(jumpToNodeButton, 1, 1);
    gridLayout->addWidget(deleteNodeButton, 1, 2);
    gridLayout->addWidget(linkNodeWithButton, 2, 1);
    gridLayout->addWidget(idSpinBox, 2,2);
    mainLayout->addLayout(gridLayout);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    formLayout = new QFormLayout();
    formLayout->addRow(commentLabel, commentField);
    formLayout->addRow(searchForLabel, searchForField);
    mainLayout->addLayout(formLayout);
    gridLayout = new QGridLayout();
    gridLayout->addWidget(findNextButton, 1 ,1);
    gridLayout->addWidget(findPreviousButton, 1, 2);
    mainLayout->addLayout(gridLayout);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(useLastRadiusBox, 1, 1);
    gridLayout->addWidget(activeNodeRadiusLabel, 2, 1);
    gridLayout->addWidget(activeNodeRadiusSpinBox, 2, 2);
    gridLayout->addWidget(defaultNodeRadiusLabel, 3, 1);
    gridLayout->addWidget(defaultNodeRadiusSpinBox, 3, 2);
    mainLayout->addLayout(gridLayout);

    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(enableCommentLockingCheckBox, 1, 1);
    gridLayout->addWidget(lockingRadiusLabel, 2, 1);
    gridLayout->addWidget(lockingRadiusSpinBox, 2, 2);
    gridLayout->addWidget(lockToNodesWithCommentLabel, 3, 1);
    gridLayout->addWidget(lockingToNodesWithCommentField, 4, 1);
    gridLayout->addWidget(lockToActiveNodeButton, 5, 1);
    gridLayout->addWidget(disableLockingButton, 5, 2);
    mainLayout->addLayout(gridLayout);

    setLayout(mainLayout);

    connect(jumpToNodeButton, SIGNAL(clicked()), this, SLOT(jumpToNodeButtonClicked()));
    connect(deleteNodeButton, SIGNAL(clicked()), this, SLOT(deleteNodeButtonClicked()));
    connect(linkNodeWithButton, SIGNAL(clicked()), this, SLOT(linkNodeWithButtonClicked()));

    connect(activeNodeXSpin, SIGNAL(valueChanged(int)), this, SLOT(activeNodeXSpinChanged(int)));
    connect(activeNodeYSpin, SIGNAL(valueChanged(int)), this, SLOT(activeNodeYSpinChanged(int)));
    connect(activeNodeZSpin, SIGNAL(valueChanged(int)), this, SLOT(activeNodeZSpinChanged(int)));

    connect(useLastRadiusBox, SIGNAL(clicked(bool)), this, SLOT(useLastRadiusChecked(bool)));
    connect(activeNodeRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(activeNodeRadiusChanged(double)));
    connect(defaultNodeRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(defaultNodeRadiusChanged(double)));
    connect(enableCommentLockingCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableCommentLockingChecked(bool)));
    connect(lockingRadiusSpinBox, SIGNAL(valueChanged(int)), this, SLOT(lockingRadiusChanged(int)));
    connect(lockingToNodesWithCommentField, SIGNAL(textChanged(QString)), this, SLOT(lockToNodesWithCommentChanged(QString)));
    connect(lockToActiveNodeButton, SIGNAL(clicked()), this, SLOT(lockToActiveNodeButtonClicked()));
    connect(disableLockingButton, SIGNAL(clicked()), this, SLOT(disableLockingButtonClicked()));

}

void ToolsNodesTabWidget::activeNodeXSpinChanged(int value) {
    if(state->skeletonState->activeNode) {
        state->skeletonState->activeNode->position.x = value;
        emit updateToolsSignal();
        emit updateTreeviewSignal();
    }
}

void ToolsNodesTabWidget::activeNodeYSpinChanged(int value) {
    if(state->skeletonState->activeNode) {
        state->skeletonState->activeNode->position.y = value;
        emit updateToolsSignal();
        emit updateTreeviewSignal();
    }
}

void ToolsNodesTabWidget::activeNodeZSpinChanged(int value) {
    if(state->skeletonState->activeNode) {
        state->skeletonState->activeNode->position.z = value;
        emit updateToolsSignal();
        emit updateTreeviewSignal();
    }
}

void ToolsNodesTabWidget::jumpToNodeButtonClicked() {
    emit jumpToNodeSignal();
}

void ToolsNodesTabWidget::deleteNodeButtonClicked() {
    emit deleteActiveNodeSignal();
    emit updateToolsSignal();
    emit updateTreeviewSignal();
}

void ToolsNodesTabWidget::linkNodeWithButtonClicked() {
    if((state->skeletonState->activeNode) and (findNodeByNodeIDSignal(this->idSpinBox->value()))) {
         emit addSegmentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, this->idSpinBox->value(), true);
    }
}

void ToolsNodesTabWidget::useLastRadiusChecked(bool on) {    
    if(on) {
        state->skeletonState->defaultNodeRadius = activeNodeIdSpinBox->value();
    } else {
        state->skeletonState->defaultNodeRadius = defaultNodeRadiusSpinBox->value();
    }
}

void ToolsNodesTabWidget::activeNodeRadiusChanged(double value) {
    if(state->skeletonState->activeNode) {
        state->skeletonState->defaultNodeRadius = value;
        state->skeletonState->activeNode->radius = value;
    }

}

void ToolsNodesTabWidget::defaultNodeRadiusChanged(double value) {
    state->skeletonState->defaultNodeRadius = value;
}

void ToolsNodesTabWidget::enableCommentLockingChecked(bool on) {
    state->skeletonState->positionLocked = on;
    if(on and lockingToNodesWithCommentField->text().isEmpty()) {
        state->viewerState->gui->lockComment = lockingToNodesWithCommentField->text();
    }
}

void ToolsNodesTabWidget::lockingRadiusChanged(int value) {
    state->skeletonState->lockRadius = value;
}

void ToolsNodesTabWidget::lockToNodesWithCommentChanged(QString comment) {
    state->viewerState->gui->lockComment = comment;
}

void ToolsNodesTabWidget::lockToActiveNodeButtonClicked() {
    Coordinate activeNodePosition;

    if(state->skeletonState->activeNode) {
        qDebug("Locking to active node");

        activeNodePosition.x = state->skeletonState->activeNode->position.x;
        activeNodePosition.y = state->skeletonState->activeNode->position.y;
        activeNodePosition.z = state->skeletonState->activeNode->position.z;

        emit lockPositionSignal(activeNodePosition);

    }
    else {
        qDebug("There is not active node to lock");
    }
}

void ToolsNodesTabWidget::disableLockingButtonClicked() {
    emit unlockPositionSignal();
}

void ToolsNodesTabWidget::updateToolsNodesTab() {
    if(state->skeletonState->activeNode) {
        activeNodeIdSpinBox->setRange(1, state->skeletonState->greatestNodeID);
        activeNodeIdSpinBox->setValue(state->skeletonState->activeNode->nodeID);
        activeNodeXSpin->setValue(state->skeletonState->activeNode->position.x);
        activeNodeYSpin->setValue(state->skeletonState->activeNode->position.y);
        activeNodeZSpin->setValue(state->skeletonState->activeNode->position.z);
        blockSignals(true);
        if(state->skeletonState->activeNode->comment) {
            commentField->setText(state->skeletonState->activeNode->comment->content);
        }
        else {
            commentField->clear();
        }
        blockSignals(false);

        activeNodeRadiusSpinBox->setValue(state->skeletonState->activeNode->radius);
    }
    else {// no active node
        activeNodeIdSpinBox->setMinimum(0);
        activeNodeIdSpinBox->setValue(0);
        activeNodeXSpin->setValue(0);
        activeNodeYSpin->setValue(0);
        activeNodeZSpin->setValue(0);
        commentField->clear();
        activeNodeRadiusSpinBox->setValue(0);
    }
    idSpinBox->setMaximum(state->skeletonState->greatestNodeID);
}

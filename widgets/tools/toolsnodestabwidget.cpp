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


#include "toolsnodestabwidget.h"
#include <QLabel>
#include <QSpinBox>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include "skeletonizer.h"

extern struct stateInfo *state;
extern struct stateInfo *tempConfig;

ToolsNodesTabWidget::ToolsNodesTabWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    activeNodeIdLabel = new QLabel("Active Node ID");
    activeNodeIdSpinBox = new QSpinBox();
    activeNodeIdSpinBox->setDisabled(true);

    jumpToNodeButton = new QPushButton("Jump to node(s)");
    deleteNodeButton = new QPushButton("Delete Node(Del)");
    linkNodeWithButton = new QPushButton("Link Node with(Shift + Click)");

    idLabel = new QLabel("ID:");
    idSpinBox = new QSpinBox();

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    commentLabel = new QLabel("Comment:");
    searchForLabel = new QLabel("Search For:");
    commentField = new QLineEdit();
    searchForField = new QLineEdit();

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    findNextButton = new QPushButton("Find (n)ext");
    findPreviousButton = new QPushButton("Find (p)revious");

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    QFrame *line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);

    useLastRadiusBox = new QCheckBox("Use Last Radius as Default");
    activeNodeRadiusLabel = new QLabel("Active Node Radius(SHIFT + wheel):");
    activeNodeRadiusSpinBox = new QDoubleSpinBox();
    defaultNodeRadiusLabel = new QLabel("Default Node Radius:");
    defaultNodeRadiusSpinBox = new QDoubleSpinBox();

    enableCommentLockingCheckBox = new QCheckBox("Enable comment locking");
    lockingRadiusLabel = new QLabel("Locking Radius:");
    lockToNodesWithCommentLabel = new QLabel("Lock To Nodes With Comment:");
    lockingRadiusSpinBox = new QSpinBox();
    lockingToNodesWithCommentField = new QLineEdit("seed");
    lockToActiveNodeButton = new QPushButton("Lock to Active Node");
    disableLockingButton = new QPushButton("Disable Locking");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(activeNodeIdLabel, activeNodeIdSpinBox);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(jumpToNodeButton, 1, 1);
    gridLayout->addWidget(deleteNodeButton, 1, 2);
    gridLayout->addWidget(linkNodeWithButton, 2, 1);

    QFormLayout *formLayout2 = new QFormLayout();
    formLayout2->addRow(idLabel, idSpinBox);

    QFormLayout *formLayout3 = new QFormLayout();
    formLayout3->addRow(commentLabel, commentField);
    formLayout3->addRow(searchForLabel, searchForField);

    QGridLayout *gridLayout2 = new QGridLayout();
    gridLayout2->addWidget(findNextButton, 1 ,1);
    gridLayout2->addWidget(findPreviousButton, 1, 2);

    QGridLayout *gridLayout3 = new QGridLayout();
    gridLayout3->addWidget(useLastRadiusBox, 1, 1);
    gridLayout3->addWidget(activeNodeRadiusLabel, 2, 1);
    gridLayout3->addWidget(activeNodeRadiusSpinBox, 2, 2);
    gridLayout3->addWidget(defaultNodeRadiusLabel, 3, 1);
    gridLayout3->addWidget(defaultNodeRadiusSpinBox, 3, 2);

    QGridLayout *gridLayout4 = new QGridLayout();
    gridLayout4->addWidget(enableCommentLockingCheckBox, 1, 1);
    gridLayout4->addWidget(lockingRadiusLabel, 2, 1);
    gridLayout4->addWidget(lockingRadiusSpinBox, 2, 2);
    gridLayout4->addWidget(lockToNodesWithCommentLabel, 3, 1);
    gridLayout4->addWidget(lockingToNodesWithCommentField, 4, 1);
    gridLayout4->addWidget(lockToActiveNodeButton, 5, 1);
    gridLayout4->addWidget(disableLockingButton, 5, 2);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(gridLayout);
    mainLayout->addLayout(formLayout2);
    mainLayout->addWidget(line);
    mainLayout->addLayout(formLayout3);
    mainLayout->addWidget(line2);
    mainLayout->addLayout(gridLayout2);
    mainLayout->addWidget(line3);
    mainLayout->addLayout(gridLayout3);
    mainLayout->addWidget(line4);
    mainLayout->addLayout(gridLayout4);

    setLayout(mainLayout);
    //mainLayout->addStretch(50);
    connect(activeNodeIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeChanged(int)));
    connect(jumpToNodeButton, SIGNAL(clicked()), this, SLOT(jumpToNodeButtonClicked()));
    connect(deleteNodeButton, SIGNAL(clicked()), this, SLOT(deleteNodeButtonClicked()));
    connect(linkNodeWithButton, SIGNAL(clicked()), this, SLOT(linkNodeWithButtonClicked()));
    connect(idSpinBox, SIGNAL(valueChanged(int)), this, SLOT(idChanged(int)));
    connect(commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(searchForField, SIGNAL(textChanged(QString)), this, SLOT(searchForChanged(QString)));
    connect(useLastRadiusBox, SIGNAL(clicked(bool)), this, SLOT(useLastRadiusChecked(bool)));
    connect(activeNodeRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(activeNodeRadiusChanged(double)));
    connect(defaultNodeRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(defaultNodeRadiusChanged(double)));
    connect(enableCommentLockingCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableCommentLockingChecked(bool)));
    connect(lockingRadiusSpinBox, SIGNAL(valueChanged(int)), this, SLOT(lockingRadiusChanged(int)));
    connect(lockingToNodesWithCommentField, SIGNAL(textChanged(QString)), this, SLOT(lockToNodesWithCommentChanged(QString)));
    connect(lockToActiveNodeButton, SIGNAL(clicked()), this, SLOT(lockToActiveNodeButtonClicked()));
    connect(disableLockingButton, SIGNAL(clicked()), this, SLOT(disableLockingButtonClicked()));


    loadSettings();
}

void ToolsNodesTabWidget::loadSettings() {
    activeNodeIdSpinBox->setValue(state->viewerState->gui->activeNodeID);
    idSpinBox->setValue(state->viewerState->gui->activeNodeID); // see todo below
    commentField->setText(QString(state->viewerState->gui->commentBuffer));
    searchForField->setText(QString(state->viewerState->gui->commentSearchBuffer));
    useLastRadiusBox->setChecked(state->viewerState->gui->useLastActNodeRadiusAsDefault);
    activeNodeRadiusSpinBox->setValue(state->viewerState->gui->actNodeRadius);
    defaultNodeRadiusSpinBox->setValue(state->skeletonState->defaultNodeRadius);
    enableCommentLockingCheckBox->setChecked(state->skeletonState->lockPositions);
    lockingRadiusSpinBox->setValue(state->skeletonState->lockRadius);
    lockingToNodesWithCommentField->setText(state->skeletonState->onCommentLock);

}

void ToolsNodesTabWidget::activeNodeChanged(int value) {
    state->viewerState->gui->activeNodeID = value;
}

/**
  * @todo why is the field above disabled and the id spinbox below?
  */
void ToolsNodesTabWidget::idChanged(int value) {
    state->viewerState->gui->activeNodeID = value;
}

void ToolsNodesTabWidget::commentChanged(QString comment) {
    state->viewerState->gui->commentBuffer = const_cast<char *>(comment.toStdString().c_str());
}

void ToolsNodesTabWidget::searchForChanged(QString comment) {
    state->viewerState->gui->commentSearchBuffer = const_cast<char *>(comment.toStdString().c_str());
}

void ToolsNodesTabWidget::jumpToNodeButtonClicked() {
    if(state->skeletonState->activeNode) {
        tempConfig->viewerState->currentPosition.x = state->skeletonState->activeNode->position.x / state->magnification;
        tempConfig->viewerState->currentPosition.y = state->skeletonState->activeNode->position.y / state->magnification;
        tempConfig->viewerState->currentPosition.z = state->skeletonState->activeNode->position.z / state->magnification;

        emit updatePositionSignal(TELL_COORDINATE_CHANGE);

    }

}

void ToolsNodesTabWidget::deleteNodeButtonClicked() {
    emit deleteActiveNodeSignal();
}

void ToolsNodesTabWidget::linkNodeWithButtonClicked() {
    if((state->skeletonState->activeNode) && (Skeletonizer::findNodeByNodeID(state->viewerState->gui->linkActiveWithNode))) {
         Skeletonizer::addSegment(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, state->viewerState->gui->linkActiveWithNode);
    }
}

/**
  * @attention the invovation of nextComment does not lead to a crash, interesting!
  */
void ToolsNodesTabWidget::findNextButtonClicked() {
    emit nextCommentSignal(state->viewerState->gui->commentSearchBuffer);
}

/**
  * @attention the invovation of nextComment does not lead to a crash, interesting!
  */
void ToolsNodesTabWidget::findPreviousButtonClicked() {
    emit previousCommentSignal(state->viewerState->gui->commentSearchBuffer);
}

void ToolsNodesTabWidget::useLastRadiusChecked(bool on) {
    state->viewerState->gui->useLastActNodeRadiusAsDefault = on;
}

void ToolsNodesTabWidget::activeNodeRadiusChanged(double value) {
    state->viewerState->gui->actNodeRadius = value;
}

void ToolsNodesTabWidget::defaultNodeRadiusChanged(double value) {
    state->skeletonState->defaultNodeRadius = value;
}

void ToolsNodesTabWidget::enableCommentLockingChecked(bool on) {
    state->skeletonState->lockPositions = on;
}

void ToolsNodesTabWidget::lockingRadiusChanged(int value) {
    state->skeletonState->lockRadius = value;
}

void ToolsNodesTabWidget::lockToNodesWithCommentChanged(QString comment) {
    strncpy(state->skeletonState->onCommentLock, comment.toStdString().c_str(), sizeof(comment.size()));
}

void ToolsNodesTabWidget::lockToActiveNodeButtonClicked() {
    Coordinate activeNodePosition;

    if(state->skeletonState->activeNode) {
        LOG("Locking to active node");

        activeNodePosition.x = state->skeletonState->activeNode->position.x;
        activeNodePosition.y = state->skeletonState->activeNode->position.y;
        activeNodePosition.z = state->skeletonState->activeNode->position.z;

        emit lockPositionSignal(activeNodePosition);

    } else {
        LOG("There is not active node to lock");
    }
}

void ToolsNodesTabWidget::disableLockingButtonClicked() {
    emit unlockPositionSignal();
}

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

#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QPushButton>
#include <QSpacerItem>

#include "knossos.h"
#include "skeletonizer.h"
#include "widgets/tools/toolstreestabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "toolsquicktabwidget.h"

extern  stateInfo *state;

ToolsQuickTabWidget::ToolsQuickTabWidget(ToolsWidget *parent) :
    QWidget(parent)
{
    // tree and node count
    treeCountLabel = new QLabel("Tree Count: 0");
    nodeCountLabel = new QLabel("Node Count: 0");
    // active tree
    this->activeTreeLabel = new QLabel("Active Tree ID:");
    this->activeTreeSpinBox = new QSpinBox();
    this->activeTreeSpinBox->setMaximum(0);
    this->activeTreeSpinBox->setMinimum(0);
    // active node
    activeNodeLabel = new QLabel("Active Node ID:");
    activeNodeSpinBox = new QSpinBox();
    activeNodeSpinBox->setMinimum(0);
    xLabel = new QLabel("x: 0");
    yLabel = new QLabel("y: 0");
    zLabel = new QLabel("z: 0");
    // node comments
    commentLabel = new QLabel("Comment:");
    commentField = new QLineEdit();
    searchForLabel = new QLabel("Search For:");
    searchForField = new QLineEdit();
    findNextButton = new QPushButton("Find (n)ext");
    findPreviousButton = new QPushButton("Find (p)revious");
    // branch points
    branchPointLabel = new QLabel("Branchpoints");
    onStackLabel = new QLabel("On Stack: 0");
    pushBranchNodeButton = new QPushButton("Push (B)ranch Node");
    popBranchNodeButton = new QPushButton("Pop && (J)ump ");
    // add to layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout;
    QFormLayout *formLayout;
    QFrame *line;
    gridLayout = new QGridLayout();
    gridLayout->addWidget(treeCountLabel, 1, 1);
    gridLayout->addWidget(nodeCountLabel, 1, 2);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);  

    formLayout = new QFormLayout();
    formLayout->addRow(activeTreeLabel, activeTreeSpinBox);
    mainLayout->addLayout(formLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    formLayout = new QFormLayout();
    formLayout->addRow(activeNodeLabel, activeNodeSpinBox);
    mainLayout->addLayout(formLayout);
    gridLayout = new QGridLayout();
    gridLayout->addWidget(xLabel, 1, 1);
    gridLayout->addWidget(yLabel, 1, 2);
    gridLayout->addWidget(zLabel, 1, 3);
    mainLayout->addLayout(gridLayout);

    formLayout = new QFormLayout();
    formLayout->addRow(commentLabel, commentField);
    formLayout->addRow(searchForLabel, searchForField);
    mainLayout->addLayout(formLayout);
    gridLayout = new QGridLayout();
    gridLayout->addWidget(findNextButton, 1, 1);
    gridLayout->addWidget(findPreviousButton, 1, 2);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    mainLayout->addWidget(branchPointLabel);
    mainLayout->addWidget(onStackLabel);
    gridLayout = new QGridLayout();
    gridLayout->addWidget(pushBranchNodeButton, 1, 1);
    gridLayout->addWidget(popBranchNodeButton, 1, 2);
    mainLayout->addLayout(gridLayout);

    mainLayout->addStretch(20);
    this->setLayout(mainLayout);
    // events on shared widgets are connected in the tools widget to prevent duplicate event handling in the tabs
    connect(pushBranchNodeButton, SIGNAL(clicked()), this, SLOT(pushBranchNodeClicked()));
    connect(popBranchNodeButton, SIGNAL(clicked()), this, SLOT(popBranchNodeClicked()));
}

void ToolsQuickTabWidget::pushBranchNodeClicked() {
    emit pushBranchNodeSignal(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0, true);
    emit updateToolsSignal();
    emit updateTreeviewSignal();
}
void ToolsQuickTabWidget::popBranchNodeClicked() {
    emit popBranchNodeSignal();
    emit updateToolsSignal();
    emit updateTreeviewSignal();
}

void ToolsQuickTabWidget::updateToolsQuickTab() {
    treeCountLabel->setText(QString("Tree Count: %1").arg(state->skeletonState->treeElements));
    nodeCountLabel->setText(QString("Node Count: %1").arg(state->skeletonState->totalNodeElements));

    if(state->skeletonState->activeTree) {
        activeTreeSpinBox->setRange(1, state->skeletonState->greatestTreeID);
        activeTreeSpinBox->setValue(state->skeletonState->activeTree->treeID);
    }
    else { // no active tree
        activeTreeSpinBox->setMinimum(0);
        activeTreeSpinBox->setValue(0);
    }
    if(state->skeletonState->activeNode) {
        activeNodeSpinBox->setRange(1, state->skeletonState->greatestNodeID);
        activeNodeSpinBox->setValue(state->skeletonState->activeNode->nodeID);
        xLabel->setText(QString("x: %1").arg(state->skeletonState->activeNode->position.x + 1));
        yLabel->setText(QString("y: %1").arg(state->skeletonState->activeNode->position.y + 1));
        zLabel->setText(QString("z: %1").arg(state->skeletonState->activeNode->position.z + 1));
        blockSignals(true);
        if(state->skeletonState->activeNode->comment) {
            commentField->setText(state->skeletonState->activeNode->comment->content);
        }
        else {
            commentField->clear();
        }
        blockSignals(false);
    }
    else { // no active node
        activeNodeSpinBox->setMinimum(0);
        activeNodeSpinBox->setValue(0);
        xLabel->setText("x: 0");
        yLabel->setText("y: 0");
        zLabel->setText("z: 0");
        commentField->clear();
    }
    // branch points on stack
    onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}

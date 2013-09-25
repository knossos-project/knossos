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

#include "toolsquicktabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QPushButton>
#include <QSpacerItem>

#include "knossos.h"

extern struct stateInfo *state;

ToolsQuickTabWidget::ToolsQuickTabWidget(ToolsWidget *parent) :
    QWidget(parent), reference(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    treeCountLabel = new QLabel("Tree Count: 0");
    nodeCountLabel = new QLabel("Node Count: 0");

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(treeCountLabel, 1, 1);
    gridLayout->addWidget(nodeCountLabel, 1, 2);
    mainLayout->addLayout(gridLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line);

    this->activeTreeLabel = new QLabel("Active Tree ID:");
    this->activeTreeSpinBox = new QSpinBox();
    this->activeTreeSpinBox->setMaximum(0);
    this->activeTreeSpinBox->setMinimum(0);    

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow(activeTreeLabel, activeTreeSpinBox);
    mainLayout->addLayout(formLayout);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line2);

    activeNodeLabel = new QLabel("Active Node ID:");
    activeNodeSpinBox = new QSpinBox();
    activeNodeSpinBox->setMinimum(0);

    QFormLayout *formLayout2 = new QFormLayout();

    formLayout2->addRow(activeNodeLabel, activeNodeSpinBox);
    mainLayout->addLayout(formLayout2);

    xLabel = new QLabel("x: 0");
    yLabel = new QLabel("y: 0");
    zLabel = new QLabel("z: 0");

    QGridLayout *gridLayout2 = new QGridLayout();
    gridLayout2->addWidget(xLabel, 1, 1);
    gridLayout2->addWidget(yLabel, 1, 2);
    gridLayout2->addWidget(zLabel, 1, 3);
    mainLayout->addLayout(gridLayout2);

    commentLabel = new QLabel("Comment:");
    commentField = new QLineEdit();

    searchForLabel = new QLabel("Search For:");
    searchForField = new QLineEdit();

    QFormLayout *formLayout3 = new QFormLayout();
    formLayout3->addRow(commentLabel, commentField);
    formLayout3->addRow(searchForLabel, searchForField);
    mainLayout->addLayout(formLayout3);

    findNextButton = new QPushButton("Find (n)ext");
    findPreviousButton = new QPushButton("Find (p)revious");

    QGridLayout *gridLayout3 = new QGridLayout();
    gridLayout3->addWidget(findNextButton, 1, 1);
    gridLayout3->addWidget(findPreviousButton, 1, 2);
    mainLayout->addLayout(gridLayout3);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(line3);

    branchPointLabel = new QLabel("Branchpoints");
    mainLayout->addWidget(branchPointLabel);

    onStackLabel = new QLabel("On Stack: 0");
    mainLayout->addWidget(onStackLabel);

    pushBranchNodeButton = new QPushButton("Push (B)ranch Node");
    popBranchNodeButton = new QPushButton("Pop & (J)ump ");

    QGridLayout *gridLayout4 = new QGridLayout();
    gridLayout4->addWidget(pushBranchNodeButton, 1, 1);
    gridLayout4->addWidget(popBranchNodeButton, 1, 2);

    mainLayout->addLayout(gridLayout4);
    mainLayout->addStretch(20);
    this->setLayout(mainLayout);

    connect(activeTreeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeTreeIdChanged(int)));
    connect(activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIdChanged(int)));
    connect(commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(searchForField, SIGNAL(textChanged(QString)), this, SLOT(searchForChanged(QString)));
    connect(findNextButton, SIGNAL(clicked()), this, SLOT(findNextButtonClicked()));
    connect(findPreviousButton, SIGNAL(clicked()), this, SLOT(findPreviousButtonClicked()));
    connect(pushBranchNodeButton, SIGNAL(clicked()), this, SLOT(pushBranchNodeButtonClicked()));
    connect(popBranchNodeButton, SIGNAL(clicked()), this, SLOT(popBranchNodeButtonClicked()));  
}


void ToolsQuickTabWidget::activeTreeIdChanged(int value) {

    qDebug() << value << "_" << activeTreeSpinBox->value();

    if(!state->skeletonState->activeTree) {
        return;
    }

    treeListElement *tree;
    if(value > state->skeletonState->activeTree->treeID) {
        while((tree = findTreeByTreeIDSignal(value)) == 0 and value <= state->skeletonState->greatestTreeID) {
            value += 1;
        }
        if(!tree) {
            activeTreeSpinBox->setValue(state->skeletonState->activeTree->treeID);
            return;
        }
    } else if(value < state->skeletonState->activeTree->treeID) {
        while((tree = findTreeByTreeIDSignal(value)) == 0 and value > 0) {
            value -= 1;
        }
        if(!tree) {
            activeTreeSpinBox->setValue(state->skeletonState->activeTree->treeID);
            return;
        }
    }

    /* As setting also the value of the activeTreeSpinBox in the TreeTabWidget(which in turn set the value of this widget). This would cause an infinite recursion.
       That´s the reason we have to disconnect the signal and reconnect it after the value it set.
       Unfortunately it´s a special case here which makes this necessary.
    */
    reference->toolsTreesTabWidget->disconnect(reference->toolsTreesTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), reference->toolsTreesTabWidget, SLOT(activeTreeIDChanged(int)));
    reference->toolsTreesTabWidget->activeTreeSpinBox->setValue(value);
    reference->toolsTreesTabWidget->connect(reference->toolsTreesTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), reference->toolsTreesTabWidget, SLOT(activeTreeIDChanged(int)));

    activeTreeSpinBox->setValue(value);
    emit setActiveTreeSignal(value);

    reference->toolsTreesTabWidget->rSpinBox->setValue(state->skeletonState->activeTree->color.r);
    reference->toolsTreesTabWidget->gSpinBox->setValue(state->skeletonState->activeTree->color.g);
    reference->toolsTreesTabWidget->bSpinBox->setValue(state->skeletonState->activeTree->color.b);
    reference->toolsTreesTabWidget->aSpinBox->setValue(state->skeletonState->activeTree->color.a);

    if(state->skeletonState->activeTree->comment)
        reference->toolsTreesTabWidget->commentField->setText(state->skeletonState->activeTree->comment);

    nodeListElement *node = state->skeletonState->activeTree->firstNode;
    if(node) {
        emit setActiveNodeSignal(CHANGE_MANUAL, node, node->nodeID);
        emit setRemoteStateTypeSignal(REMOTE_RECENTERING);
        emit setRecenteringPositionSignal(state->skeletonState->activeNode->position.x / state->magnification,
                                       state->skeletonState->activeNode->position.y / state->magnification,
                                       state->skeletonState->activeNode->position.z / state->magnification);
        emit Knossos::sendRemoteSignal();
    }
}

void ToolsQuickTabWidget::activeNodeIdChanged(int value) {
    if(!state->skeletonState->activeNode)
        return;

    nodeListElement *node;

    if(value > state->skeletonState->activeNode->nodeID) {
        while((node = findNodeByNodeIDSignal(value)) == 0 and value <= state->skeletonState->greatestNodeID) {
            value += 1;
        }


        disconnect(this->activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIdChanged(int)));
        disconnect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));
        this->activeNodeSpinBox->setValue(value);
        reference->toolsNodesTabWidget->activeNodeIdSpinBox->setValue(value);
        connect(this->activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIdChanged(int)));
        connect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));


        if(!node) {
            return;
        }
    } else if(value < state->skeletonState->activeNode->nodeID) {
        while((node = findNodeByNodeIDSignal(value)) == 0 and value > 0) {
            value -= 1;
        }

        disconnect(this->activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIdChanged(int)));
        disconnect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));
        this->activeNodeSpinBox->setValue(value);
        reference->toolsNodesTabWidget->activeNodeIdSpinBox->setValue(value);
        connect(this->activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeIdChanged(int)));
        connect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));


        if(!node) {
            return;
        }
    }

    if(setActiveNodeSignal(CHANGE_MANUAL, 0, value)) {

        if(state->skeletonState->activeNode) {
            this->xLabel->setText(QString("x: %1").arg(state->skeletonState->activeNode->position.x));
            this->yLabel->setText(QString("y: %1").arg(state->skeletonState->activeNode->position.y));
            this->zLabel->setText(QString("z: %1").arg(state->skeletonState->activeNode->position.z));

            reference->toolsNodesTabWidget->activeNodeIdSpinBox->disconnect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));
            reference->toolsNodesTabWidget->activeNodeIdSpinBox->setValue(state->skeletonState->activeNode->nodeID);
            reference->toolsNodesTabWidget->activeNodeIdSpinBox->connect(reference->toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), reference->toolsNodesTabWidget, SLOT(activeNodeChanged(int)));

            if(state->skeletonState->activeNode->comment and state->skeletonState->activeNode->comment->content) {
                this->commentField->setText(QString(state->skeletonState->activeNode->comment->content));
                reference->toolsNodesTabWidget->commentField->setText(QString(state->skeletonState->activeNode->comment->content));
            } else {
                this->commentField->setText("");
                reference->toolsNodesTabWidget->commentField->setText("");
            }
        }
    } else {

    }
}

void ToolsQuickTabWidget::commentChanged(QString comment) {
    char *ccomment = const_cast<char *>(comment.toStdString().c_str());
    if((!state->skeletonState->activeNode->comment) && (strncmp(ccomment, "", 1) != 0)){

        emit addCommentSignal(CHANGE_MANUAL, ccomment, state->skeletonState->activeNode, 0, true);
    }
    else{
        if(!comment.isEmpty())
            emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0, ccomment, state->skeletonState->activeNode, 0, true);
    }

    emit updateCommentsTableSignal();
    this->commentField->setText(comment);
    reference->toolsNodesTabWidget->commentField->setText(comment);

}

void ToolsQuickTabWidget::searchForChanged(QString comment) {
    reference->toolsNodesTabWidget->searchForField->setText(comment);
}

void ToolsQuickTabWidget::findNextButtonClicked() {
    char *searchStr = const_cast<char *>(this->searchForField->text().toStdString().c_str());
    emit nextCommentSignal(searchStr);
}

void ToolsQuickTabWidget::findPreviousButtonClicked() {
    char *searchStr = const_cast<char *>(this->searchForField->text().toStdString().c_str());
    emit previousCommentSignal(searchStr);
}

void ToolsQuickTabWidget::pushBranchNodeButtonClicked() {
    emit pushBranchNodeSignal(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0, true);
    this->onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}

void ToolsQuickTabWidget::popBranchNodeButtonClicked() {
    emit popBranchNodeSignal(CHANGE_MANUAL, true);
    this->onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}



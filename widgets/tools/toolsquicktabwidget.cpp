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

#include <QVBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFrame>
#include <QPushButton>
#include <QSpacerItem>

#include "skeletonizer.h"

extern struct stateInfo *state;

ToolsQuickTabWidget::ToolsQuickTabWidget(ToolsWidget *parent) :
    QWidget(parent), ref(parent)
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
    activeNodeSpinBox->setMinimum(1);

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

    Skeletonizer::setActiveTreeByID(value);
    /*
    int currentIndex = state->skeletonState->activeTree->treeID - 1;
    int nextIndex = ref->findTreeIndex(value);

    if(currentIndex < nextIndex and nextIndex < ref->trees->size()) {
        //activeTreeSpinBox->setValue(ref->trees->at(nextIndex + 1));
        emit setActiveTreeSignal(ref->trees->at(currentIndex + 1));
        this->activeTreeSpinBox->blockSignals(true);
        this->activeTreeSpinBox->setValue(ref->trees->at(currentIndex+1));
        this->activeTreeSpinBox->blockSignals(false);
    } else if(currentIndex > nextIndex and nextIndex >= 0) {
        //activeTreeSpinBox->setValue(ref->trees->at(nextIndex - 1));
        emit setActiveTreeSignal(ref->trees->at(currentIndex - 1));
        this->activeTreeSpinBox->blockSignals(true);
        this->activeTreeSpinBox->setValue(ref->trees->at(currentIndex-1));
        this->activeTreeSpinBox->blockSignals(false);
    } else if(currentIndex == nextIndex) {
        //emit setActiveTreeSignal(ref->trees->at(value));
    }
    */

    /*
    ref->toolsTreesTabWidget->blockSignals(true);
    ref->toolsTreesTabWidget->activeTreeSpinBox->setValue(this->activeTreeSpinBox->value());
    ref->toolsTreesTabWidget->blockSignals(false);
    */
    //emit updateToolsSignal();
}

void ToolsQuickTabWidget::activeNodeIdChanged(int value) {
    /*
    int currentIndex = state->skeletonState->activeNode->nodeID - 1;
    int nextIndex = ref->findTreeIndex(value);

    if(currentIndex < nextIndex and nextIndex < ref->trees->size()) {
        //activeTreeSpinBox->setValue(ref->trees->at(nextIndex + 1));
        emit setActiveTreeSignal(ref->trees->at(currentIndex + 1));
        this->activeTreeSpinBox->blockSignals(true);
        this->activeTreeSpinBox->setValue(ref->trees->at(currentIndex+1));
        this->activeTreeSpinBox->blockSignals(false);
    } else if(currentIndex > nextIndex and nextIndex >= 0) {
        //activeTreeSpinBox->setValue(ref->trees->at(nextIndex - 1));
        emit setActiveTreeSignal(ref->trees->at(currentIndex - 1));
        this->activeTreeSpinBox->blockSignals(true);
        this->activeTreeSpinBox->setValue(ref->trees->at(currentIndex-1));
        this->activeTreeSpinBox->blockSignals(false);
    } else if(currentIndex == nextIndex) {
        //emit setActiveTreeSignal(ref->trees->at(value));
    }
    */


    qDebug() << " hier node changed";
    emit setActiveNodeSignal(CHANGE_MANUAL, 0, value);

    if(state->skeletonState->activeNode) {
        this->xLabel->setText(QString("x: %1").arg(state->skeletonState->activeNode->position.x));
        this->yLabel->setText(QString("y: %1").arg(state->skeletonState->activeNode->position.y));
        this->zLabel->setText(QString("z: %1").arg(state->skeletonState->activeNode->position.z));

        if(state->skeletonState->activeNode->comment and state->skeletonState->activeNode->comment->content)
            this->commentField->setText(QString(state->skeletonState->activeNode->comment->content));
        else
            this->commentField->clear();
    }
}

void ToolsQuickTabWidget::commentChanged(QString comment) {
    if(state->skeletonState->activeNode) {
        state->skeletonState->activeNode->comment->content = const_cast<char *>(comment.toStdString().c_str());
    }
}

void ToolsQuickTabWidget::searchForChanged(QString comment) {
    state->viewerState->gui->commentSearchBuffer = const_cast<char *>(comment.toStdString().c_str());
}


void ToolsQuickTabWidget::findNextButtonClicked() {
    qDebug() << state->viewerState->gui->commentSearchBuffer;
    emit nextCommentSignal(state->viewerState->gui->commentSearchBuffer);
}


void ToolsQuickTabWidget::findPreviousButtonClicked() {
    emit previousCommentSignal(state->viewerState->gui->commentSearchBuffer);
}

void ToolsQuickTabWidget::pushBranchNodeButtonClicked() {
    Skeletonizer::pushBranchNode(CHANGE_MANUAL, true, true, state->skeletonState->activeNode, 0);
    this->onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}


void ToolsQuickTabWidget::popBranchNodeButtonClicked() {
    Skeletonizer::popBranchNode(CHANGE_MANUAL);
    this->onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
}



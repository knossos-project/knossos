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

#include "toolstreestabwidget.h"
#include "widgets/tools/toolsquicktabwidget.h"

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>

#include "skeletonizer.h"

extern struct stateInfo *state;

ToolsTreesTabWidget::ToolsTreesTabWidget(ToolsWidget *parent) :
    QWidget(parent), ref(parent)
{

    activeTreeLabel = new QLabel("Active Tree ID:");
    activeTreeSpinBox = new QSpinBox();
    this->activeTreeSpinBox->setMaximum(0);
    this->activeTreeSpinBox->setMinimum(0);

    QVBoxLayout *mainLayout = new QVBoxLayout();

    deleteActiveTreeButton = new QPushButton("Delete Active Tree");
    newTreeButton = new QPushButton("New Tree (C)");

    commentLabel = new QLabel("Comment");
    commentField = new QLineEdit();

    mergeTreesButton = new QPushButton("Merge Trees");
    id1Label = new QLabel("ID 1:");
    id1SpinBox = new QSpinBox();
    id1SpinBox->setMinimum(1);
    id2Label = new QLabel("ID 2:");
    id2SpinBox = new QSpinBox();
    id2SpinBox->setMinimum(1);

    splitByConnectedComponentsButton = new QPushButton("Split By Connected Components");
    rLabel = new QLabel("R:");
    gLabel = new QLabel("G:");
    bLabel = new QLabel("B:");
    aLabel = new QLabel("A:");

    rSpinBox = new QDoubleSpinBox();
    rSpinBox->setSingleStep(0.01);
    rSpinBox->setMaximum(1);
    gSpinBox = new QDoubleSpinBox();
    gSpinBox->setSingleStep(0.01);
    gSpinBox->setMaximum(1);
    bSpinBox = new QDoubleSpinBox();
    bSpinBox->setSingleStep(0.01);
    bSpinBox->setMaximum(1);
    aSpinBox = new QDoubleSpinBox();
    aSpinBox->setSingleStep(0.01);
    aSpinBox->setMaximum(1);

    mergeTreesButton = new QPushButton("Merge Trees");
    this->restoreDefaultColorButton = new QPushButton("Restore default color");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    QFrame *line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);

    QFrame *line5 = new QFrame();
    line5->setFrameShape(QFrame::HLine);
    line5->setFrameShadow(QFrame::Sunken);

    QFrame *line6 = new QFrame();
    line6->setFrameShape(QFrame::HLine);
    line6->setFrameShadow(QFrame::Sunken);

    QGridLayout *gridLayout = new QGridLayout();

    gridLayout->addWidget(activeTreeLabel, 1, 1);
    gridLayout->addWidget(activeTreeSpinBox, 1, 2);

    mainLayout->addLayout(gridLayout);
    mainLayout->addWidget(line);

    QGridLayout *gridLayout2 = new QGridLayout();

    gridLayout2->addWidget(deleteActiveTreeButton, 2, 1);
    gridLayout2->addWidget(newTreeButton, 2, 2);
    mainLayout->addLayout(gridLayout2);
    mainLayout->addWidget(line2);

    QGridLayout *gridLayout3 = new QGridLayout();
    gridLayout3->addWidget(commentLabel, 1, 1);
    gridLayout3->addWidget(commentField, 1, 2);
    mainLayout->addLayout(gridLayout3);
    mainLayout->addWidget(line3);

    QGridLayout *gridLayout4 = new QGridLayout();
    gridLayout4->addWidget(mergeTreesButton, 1, 1);
    gridLayout4->addWidget(id1Label, 1, 2);
    gridLayout4->addWidget(id1SpinBox, 1, 3);
    gridLayout4->addWidget(id2Label, 1, 4);
    gridLayout4->addWidget(id2SpinBox, 1, 5);

    mainLayout->addLayout(gridLayout4);
    mainLayout->addWidget(line4);
    mainLayout->addWidget(splitByConnectedComponentsButton);
    mainLayout->addWidget(line5);

    QGridLayout *gridLayout5 = new QGridLayout();
    gridLayout5->addWidget(rLabel, 1, 1);
    gridLayout5->addWidget(rSpinBox, 1, 2);
    gridLayout5->addWidget(gLabel, 1, 3);
    gridLayout5->addWidget(gSpinBox, 1, 4);
    gridLayout5->addWidget(bLabel, 1, 5);
    gridLayout5->addWidget(bSpinBox, 1, 6);
    gridLayout5->addWidget(aLabel, 1, 7);
    gridLayout5->addWidget(aSpinBox, 1, 8);
    mainLayout->addLayout(gridLayout5);
    mainLayout->addWidget(restoreDefaultColorButton);

    mainLayout->addStretch(20);
    setLayout(mainLayout);

    connect(activeTreeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeTreeIDChanged(int)));
    connect(deleteActiveTreeButton, SIGNAL(clicked()), this, SLOT(deleteActiveTreeButtonClicked()));
    connect(newTreeButton, SIGNAL(clicked()), this, SLOT(newTreeButtonClicked()));
    connect(commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(mergeTreesButton, SIGNAL(clicked()), this, SLOT(mergeTreesButtonClicked()));
    connect(rSpinBox, SIGNAL(valueChanged(double)), this, SLOT(rChanged(double)));
    connect(gSpinBox, SIGNAL(valueChanged(double)), this, SLOT(gChanged(double)));
    connect(bSpinBox, SIGNAL(valueChanged(double)), this, SLOT(bChanged(double)));
    connect(aSpinBox, SIGNAL(valueChanged(double)), this, SLOT(aChanged(double)));
    connect(restoreDefaultColorButton, SIGNAL(clicked()), this, SLOT(restoreDefaultColorButtonClicked()));


}

void ToolsTreesTabWidget::activeTreeIDChanged(int value) {
    if(!state->skeletonState->activeTree)
        return;

    qDebug() << "tt activeTree changed begin";

    treeListElement *tree;
    if(value > state->skeletonState->activeTree->treeID) {
        while((tree = Skeletonizer::findTreeByTreeID(value)) == 0 and value <= state->skeletonState->greatestTreeID) {
            value += 1;
        }
        if(!tree) {
            return;
        }
    } else if(value < state->skeletonState->activeTree->treeID) {
        while((tree = Skeletonizer::findTreeByTreeID(value)) == 0 and value > 0) {
            value -= 1;
        }
        if(!tree) {
            return;
        }
    }

    /* This prevents the infinite recursion. See QuickTabWidget for more information */
    ref->toolsQuickTabWidget->disconnect(ref->toolsQuickTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), ref->toolsQuickTabWidget, SLOT(activeTreeIdChanged(int)));
    ref->toolsQuickTabWidget->activeTreeSpinBox->setValue(value);
    ref->toolsQuickTabWidget->connect(ref->toolsQuickTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), ref->toolsQuickTabWidget, SLOT(activeTreeIdChanged(int)));

    activeTreeSpinBox->setValue(value);

    Skeletonizer::setActiveTreeByID(value);

    rSpinBox->setValue(state->skeletonState->activeTree->color.r);
    gSpinBox->setValue(state->skeletonState->activeTree->color.g);
    bSpinBox->setValue(state->skeletonState->activeTree->color.b);
    aSpinBox->setValue(state->skeletonState->activeTree->color.a);

    if(state->skeletonState->activeTree->comment)
        commentField->setText(state->skeletonState->activeTree->comment);

    nodeListElement *node = state->skeletonState->activeTree->firstNode;
    if(node) {
        emit (CHANGE_MANUAL, node, node->nodeID);
        /* @todo set_coordinate */

        /* @todo send_remote signal */
    }


    qDebug() << "tt active Tree changed end";
}

void ToolsTreesTabWidget::deleteActiveTreeButtonClicked() {
    int retValue = QMessageBox::warning(this, "Warning", "Do you really want to delete the active tree", QMessageBox::Yes, QMessageBox::No);
    switch(retValue) {
        case QMessageBox::Yes:
            emit delActiveTreeSignal();
            emit updateToolsSignal();

    }
}

void ToolsTreesTabWidget::newTreeButtonClicked() {
    color4F treeCol;
    treeCol.r = -1;
    treeCol.g = -1;
    treeCol.b = -1;
    treeCol.a = 1;
    Skeletonizer::addTreeListElement(true, CHANGE_MANUAL, 0, treeCol);
    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    emit updateToolsSignal();
}

void ToolsTreesTabWidget::commentChanged(QString comment) {
    if(state->skeletonState->activeTree and !comment.isEmpty()) {
        qDebug() << comment.toStdString().c_str();
        Skeletonizer::addTreeComment(CHANGE_MANUAL, state->skeletonState->activeTree->treeID, const_cast<char *>(comment.toStdString().c_str()));
    } else {

    }
}

void ToolsTreesTabWidget::mergeTreesButtonClicked() {    
    qDebug() << id1SpinBox->value() << " [] " << id2SpinBox->value();
    if(Skeletonizer::mergeTrees(CHANGE_MANUAL, id1SpinBox->value(), id2SpinBox->value())) {
        ref->updateDisplayedTree();

    } else {
        LOG("Probleme");
    }
}

void ToolsTreesTabWidget::id1Changed(int value) {
    state->viewerState->gui->mergeTreesID1 = value;
}

void ToolsTreesTabWidget::id2Changed(int value) {
    state->viewerState->gui->mergeTreesID2 = value;
}

void ToolsTreesTabWidget::splitByConnectedComponentsButtonClicked() {
    if(state->skeletonState->activeNode) {
        if(Skeletonizer::splitConnectedComponent(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID)) {
            ref->updateDisplayedTree();
        } else {
            LOG("Probleme");
        }
    }
}

void ToolsTreesTabWidget::rChanged(double value) {
    state->viewerState->gui->actTreeColor.r = value;
    state->skeletonState->activeTree->color.r = state->viewerState->gui->actTreeColor.r;
}

void ToolsTreesTabWidget::gChanged(double value) {
    state->viewerState->gui->actTreeColor.g = value;
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.g = state->viewerState->gui->actTreeColor.g;
}

void ToolsTreesTabWidget::bChanged(double value) {
    state->viewerState->gui->actTreeColor.b = value;
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.b = state->viewerState->gui->actTreeColor.b;
}

void ToolsTreesTabWidget::aChanged(double value) {
    state->viewerState->gui->actTreeColor.a = value;
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.a = state->viewerState->gui->actTreeColor.a;
}

void ToolsTreesTabWidget::restoreDefaultColorButtonClicked() {
    Skeletonizer::restoreDefaultTreeColor();
    rSpinBox->setValue(0);
    gSpinBox->setValue(0);
    bSpinBox->setValue(0);
    aSpinBox->setValue(0);
}



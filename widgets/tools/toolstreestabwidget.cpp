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
#include <QMessageBox>

#include "knossos.h"

extern struct stateInfo *state;

ToolsTreesTabWidget::ToolsTreesTabWidget(ToolsWidget *parent) :
    QWidget(parent)
{

    activeTreeLabel = new QLabel("Active Tree ID:");
    activeTreeSpinBox = new QSpinBox();
    activeTreeSpinBox->setMinimum(0);
    activeTreeSpinBox->setMaximum(0);
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

    splitByConnectedComponentsButton = new QPushButton("Split Last Component From Tree");
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

    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout;
    QFrame *line;

    gridLayout = new QGridLayout();
    gridLayout->addWidget(activeTreeLabel, 1, 1);
    gridLayout->addWidget(activeTreeSpinBox, 1, 2);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(deleteActiveTreeButton, 2, 1);
    gridLayout->addWidget(newTreeButton, 2, 2);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(commentLabel, 1, 1);
    gridLayout->addWidget(commentField, 1, 2);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(mergeTreesButton, 1, 1);
    gridLayout->addWidget(id1Label, 1, 2);
    gridLayout->addWidget(id1SpinBox, 1, 3);
    gridLayout->addWidget(id2Label, 1, 4);
    gridLayout->addWidget(id2SpinBox, 1, 5);
    mainLayout->addLayout(gridLayout);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    mainLayout->addWidget(splitByConnectedComponentsButton);
    line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);

    gridLayout = new QGridLayout();
    gridLayout->addWidget(rLabel, 1, 1);
    gridLayout->addWidget(rSpinBox, 1, 2);
    gridLayout->addWidget(gLabel, 1, 3);
    gridLayout->addWidget(gSpinBox, 1, 4);
    gridLayout->addWidget(bLabel, 1, 5);
    gridLayout->addWidget(bSpinBox, 1, 6);
    gridLayout->addWidget(aLabel, 1, 7);
    gridLayout->addWidget(aSpinBox, 1, 8);
    mainLayout->addLayout(gridLayout);

    mainLayout->addWidget(restoreDefaultColorButton);

    mainLayout->addStretch(20);
    setLayout(mainLayout);

    connect(deleteActiveTreeButton, SIGNAL(clicked()), this, SLOT(deleteActiveTreeButtonClicked()));
    connect(newTreeButton, SIGNAL(clicked()), this, SLOT(newTreeButtonClicked()));
    connect(commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(mergeTreesButton, SIGNAL(clicked()), this, SLOT(mergeTreesButtonClicked()));
    connect(splitByConnectedComponentsButton, SIGNAL(clicked()), this, SLOT(splitByConnectedComponentsButtonClicked()));
    connect(rSpinBox, SIGNAL(valueChanged(double)), this, SLOT(rChanged(double)));
    connect(gSpinBox, SIGNAL(valueChanged(double)), this, SLOT(gChanged(double)));
    connect(bSpinBox, SIGNAL(valueChanged(double)), this, SLOT(bChanged(double)));
    connect(aSpinBox, SIGNAL(valueChanged(double)), this, SLOT(aChanged(double)));
    connect(restoreDefaultColorButton, SIGNAL(clicked()), this, SLOT(restoreDefaultColorButtonClicked()));
}

void ToolsTreesTabWidget::deleteActiveTreeButtonClicked() {
    QMessageBox warning;
    warning.setWindowFlags(Qt::WindowStaysOnTopHint);
    warning.setWindowTitle("Warning");
    warning.setText("Do you really want to delete the active tree?");
    QPushButton *yes = warning.addButton(QMessageBox::Yes);
    warning.addButton(QMessageBox::No);
    warning.exec();
    if(warning.clickedButton() == yes) {
        emit delActiveTreeSignal();
        emit updateToolsSignal();
        emit updateTreeviewSignal();
    }
}

void ToolsTreesTabWidget::deleteActiveTreeWithoutConfirmation() {
    emit delActiveTreeSignal();
    emit updateToolsSignal();
    emit updateTreeviewSignal();
}

void ToolsTreesTabWidget::newTreeButtonClicked() {
    color4F treeCol;
    treeCol.r = -1;
    treeCol.g = -1;
    treeCol.b = -1;
    treeCol.a = 1;
    emit addTreeListElement(true, CHANGE_MANUAL, 0, treeCol, true);
    state->skeletonState->workMode = SKELETONIZER_ON_CLICK_ADD_NODE;
    emit updateTreeviewSignal();
    emit updateToolsSignal();
    emit updateTreeviewSignal();
}

void ToolsTreesTabWidget::commentChanged(QString comment) {
    if(state->skeletonState->activeTree and !comment.isEmpty()) {
        //qDebug() << comment.toStdString().c_str();
        emit addTreeComment(CHANGE_MANUAL, state->skeletonState->activeTree->treeID, const_cast<char *>(comment.toStdString().c_str()));
    }
}

void ToolsTreesTabWidget::mergeTreesButtonClicked() {    
    qDebug() << id1SpinBox->value() << " [] " << id2SpinBox->value();
    if(mergeTrees(CHANGE_MANUAL, id1SpinBox->value(), id2SpinBox->value(), true)) {
        emit updateToolsSignal();
        emit updateTreeviewSignal();
    }
}


void ToolsTreesTabWidget::splitByConnectedComponentsButtonClicked() {
    if(state->skeletonState->activeNode) {
        if(splitConnectedComponent(CHANGE_MANUAL, state->skeletonState->activeNode->nodeID, true)) {
            emit updateToolsSignal();
            emit updateTreeviewSignal();
        }
    }
}

void ToolsTreesTabWidget::rChanged(double value) {
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.r = value;

}

void ToolsTreesTabWidget::gChanged(double value) {
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.g = value;
}

void ToolsTreesTabWidget::bChanged(double value) {
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.b = value;
}

void ToolsTreesTabWidget::aChanged(double value) {
    if(state->skeletonState->activeTree)
        state->skeletonState->activeTree->color.a = value;
}

void ToolsTreesTabWidget::restoreDefaultColorButtonClicked() {
    //emit restoreDefaultTreeColorSignal();
    rSpinBox->setValue(0);
    gSpinBox->setValue(0);
    bSpinBox->setValue(0);
    aSpinBox->setValue(0);
}

void ToolsTreesTabWidget::updateToolsTreesTab() {
    if(state->skeletonState->activeTree) {
        activeTreeSpinBox->setRange(1, state->skeletonState->greatestTreeID);
        activeTreeSpinBox->setValue(state->skeletonState->activeTree->treeID);
        blockSignals(true);
        if(state->skeletonState->activeTree->comment) {
            commentField->setText(state->skeletonState->activeTree->comment);
        }
        else {
            commentField->clear();
        }
        blockSignals(false);
        rSpinBox->setValue(state->skeletonState->activeTree->color.r);
        gSpinBox->setValue(state->skeletonState->activeTree->color.g);
        bSpinBox->setValue(state->skeletonState->activeTree->color.b);
        aSpinBox->setValue(state->skeletonState->activeTree->color.a);
        return;
    }
    // no active tree
    activeTreeSpinBox->setMinimum(0);
    activeTreeSpinBox->setValue(0);
    commentField->clear();
    rSpinBox->setValue(0);
    gSpinBox->setValue(0);
    bSpinBox->setValue(0);
    aSpinBox->setValue(0);
}

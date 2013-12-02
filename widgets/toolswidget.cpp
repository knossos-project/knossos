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

#include <QSettings>

#include "knossos-global.h"
#include "skeletonizer.h"
#include "toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"
#include "GUIConstants.h"

extern stateInfo *state;

ToolsWidget::ToolsWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tools");
    tabs = new QTabWidget(this);

    this->toolsQuickTabWidget = new ToolsQuickTabWidget(this);
    this->toolsNodesTabWidget = new ToolsNodesTabWidget(this);
    this->toolsTreesTabWidget = new ToolsTreesTabWidget(this);

    tabs->addTab(toolsQuickTabWidget, "Quick");
    tabs->addTab(toolsTreesTabWidget, "Trees");
    tabs->addTab(toolsNodesTabWidget, "Nodes");
    //toggleAllWidgets(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);

    // signals and slots for quick tab widget are handled here to prevent duplicate event handling in the tabs
    // active tree and active node changes
    connect(toolsQuickTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeTreeSpinChanged(int)));
    connect(toolsTreesTabWidget->activeTreeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeTreeSpinChanged(int)));
    connect(toolsQuickTabWidget->activeNodeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeSpinChanged(int)));
    connect(toolsNodesTabWidget->activeNodeIdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(activeNodeSpinChanged(int)));
    // comment changes, comment searches
    connect(toolsQuickTabWidget->commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(toolsNodesTabWidget->commentField, SIGNAL(textChanged(QString)), this, SLOT(commentChanged(QString)));
    connect(toolsQuickTabWidget->searchForField, SIGNAL(textChanged(QString)), this, SLOT(searchForChanged(QString)));
    connect(toolsNodesTabWidget->searchForField, SIGNAL(textChanged(QString)), this, SLOT(searchForChanged(QString)));
    connect(toolsQuickTabWidget->findNextButton, SIGNAL(clicked()), this, SLOT(findNextCommentClicked()));
    connect(toolsNodesTabWidget->findNextButton, SIGNAL(clicked()), this, SLOT(findNextCommentClicked()));
    connect(toolsQuickTabWidget->findPreviousButton, SIGNAL(clicked()), this, SLOT(findPreviousCommentClicked()));
    connect(toolsNodesTabWidget->findPreviousButton, SIGNAL(clicked()), this, SLOT(findPreviousCommentClicked()));

    connect(toolsQuickTabWidget, SIGNAL(updateToolsSignal()), this, SLOT(updateToolsSlot()));
    connect(toolsTreesTabWidget, SIGNAL(updateToolsSignal()), this, SLOT(updateToolsSlot()));
    connect(toolsNodesTabWidget, SIGNAL(updateToolsSignal()), this, SLOT(updateToolsSlot()));
}

void ToolsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}

void ToolsWidget::activeTreeSpinChanged(int value) {
    treeListElement *tree = findTreeByTreeIDSignal(value);
    if(tree == NULL) {
        // no tree with this value.
        // jump to next or previous tree, depending on wether spin box value is greater or smaller than active tree id
        if(state->skeletonState->activeTree == NULL) { // no active node, activate first node found
            tree = Skeletonizer::getTreeWithNextID(NULL);
        }
        else {
            if(value > state->skeletonState->activeTree->treeID) { // move to next node
                tree = Skeletonizer::getTreeWithNextID(state->skeletonState->activeTree);
            }
            else if(value < state->skeletonState->activeTree->treeID) {
                tree = Skeletonizer::getTreeWithPrevID(state->skeletonState->activeTree);
            }
        }
    }
    if(tree) {
        setActiveTreeSignal(tree->treeID);
    }
    updateToolsSlot();
    return;
}

void ToolsWidget::activeNodeSpinChanged(int value) {
    nodeListElement *node = findNodeByNodeIDSignal(value);
    if(node == NULL) {
        // no node with this value.
        // jump to next or previous node, depending on wether spin box value is greater or smaller than active node id
        if(state->skeletonState->activeNode == NULL) { // no active node, activate first node found
            node = Skeletonizer::getNodeWithNextID(NULL, false);
        }
        else {
            if((uint)value > state->skeletonState->activeNode->nodeID) { // move to next node
                node = Skeletonizer::getNodeWithNextID(state->skeletonState->activeNode, false);
            }
            else if((uint)value < state->skeletonState->activeNode->nodeID) {
                node = Skeletonizer::getNodeWithPrevID(state->skeletonState->activeNode, false);
            }
        }
    }
    if(node) {
        setActiveNodeSignal(CHANGE_MANUAL, node, 0);
    }
    updateToolsSlot();
    return;
}

void ToolsWidget::commentChanged(QString comment) {
    if(state->skeletonState->activeNode == NULL) {
        return;
    }
    if(state->skeletonState->activeNode->comment == NULL && comment.length() > 0){
        emit addCommentSignal(CHANGE_MANUAL, comment.toLocal8Bit().data(), state->skeletonState->activeNode, 0, true);
    }
    else if(comment.isEmpty() == false) {
        emit editCommentSignal(CHANGE_MANUAL, state->skeletonState->activeNode->comment, 0,
                               comment.toLocal8Bit().data(), state->skeletonState->activeNode, 0, true);
    }
    emit updateCommentsTableSignal();
    updateToolsSlot();
}

void ToolsWidget::searchForChanged(QString searchString) {
    toolsNodesTabWidget->searchForField->setText(searchString);
    toolsQuickTabWidget->searchForField->setText(searchString);
}

void ToolsWidget::findPreviousCommentClicked() {
    emit previousCommentSignal(toolsNodesTabWidget->searchForField->text().toLocal8Bit().data());
}

void ToolsWidget::findNextCommentClicked() {
    emit nextCommentSignal(toolsNodesTabWidget->searchForField->text().toLocal8Bit().data());
}

void ToolsWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();

    /* Should n be saved */
    this->toolsQuickTabWidget->activeTreeSpinBox->setValue(settings.value(ACTIVE_TREE_ID).toInt());
    this->toolsQuickTabWidget->activeNodeSpinBox->setValue(settings.value(ACTIVE_NODE_ID).toInt());
    this->toolsQuickTabWidget->commentField->setText(settings.value(COMMENT).toString());
    this->toolsQuickTabWidget->searchForField->setText(settings.value(COMMENT).toString());

    this->toolsTreesTabWidget->commentField->setText(settings.value(TREE_COMMENT).toString());
    this->toolsTreesTabWidget->id1SpinBox->setValue(settings.value(ID1).toInt());
    this->toolsTreesTabWidget->id2SpinBox->setValue(settings.value(ID2).toInt());
    this->toolsTreesTabWidget->rSpinBox->setValue(settings.value(R).toDouble());
    this->toolsTreesTabWidget->gSpinBox->setValue(settings.value(G).toDouble());
    this->toolsTreesTabWidget->bSpinBox->setValue(settings.value(B).toDouble());
    this->toolsTreesTabWidget->aSpinBox->setValue(settings.value(A).toDouble());

    this->toolsNodesTabWidget->commentField->setText(settings.value(COMMENT).toString());
    if(!settings.value(SEARCH_FOR).toString().isNull())
        this->toolsNodesTabWidget->searchForField->setText(settings.value(SEARCH_FOR).toString());
    this->toolsNodesTabWidget->useLastRadiusBox->setChecked(settings.value(USE_LAST_RADIUS_AS_DEFAULT).toBool());

    if(!settings.value(ACTIVE_NODE_RADIUS).isNull())
        this->toolsNodesTabWidget->activeNodeRadiusSpinBox->setValue(settings.value(ACTIVE_NODE_RADIUS).toDouble());
    else
        this->toolsNodesTabWidget->activeNodeRadiusSpinBox->setValue(1);

    if(!settings.value(DEFAULT_NODE_RADIUS).isNull())
        this->toolsNodesTabWidget->defaultNodeRadiusSpinBox->setValue(settings.value(DEFAULT_NODE_RADIUS).toDouble());
    else
        this->toolsNodesTabWidget->defaultNodeRadiusSpinBox->setValue(1);

    this->toolsNodesTabWidget->enableCommentLockingCheckBox->setChecked(settings.value(ENABLE_COMMENT_LOCKING).toBool());
    this->toolsNodesTabWidget->lockingRadiusSpinBox->setValue(settings.value(LOCKING_RADIUS).toInt());
    if(!settings.value(LOCK_TO_NODES_WITH_COMMENT).toString().isNull())
        this->toolsNodesTabWidget->lockingToNodesWithCommentField->setText(settings.value(LOCK_TO_NODES_WITH_COMMENT).toString());

    settings.endGroup();

    setGeometry(x, y, width, height);
}

void ToolsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(TOOLS_WIDGET);

    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(ACTIVE_TREE_ID, this->toolsQuickTabWidget->activeTreeSpinBox->value());
    settings.setValue(ACTIVE_NODE_ID, this->toolsQuickTabWidget->activeNodeSpinBox->value());
    settings.setValue(COMMENT, this->toolsQuickTabWidget->commentField->text());
    settings.setValue(SEARCH_FOR, this->toolsQuickTabWidget->searchForField->text());

    settings.setValue(TREE_COMMENT, this->toolsTreesTabWidget->commentField->text());
    settings.setValue(ID1, this->toolsTreesTabWidget->id1SpinBox->value());
    settings.setValue(ID2, this->toolsTreesTabWidget->id2SpinBox->value());
    settings.setValue(R, this->toolsTreesTabWidget->rSpinBox->value());
    settings.setValue(G, this->toolsTreesTabWidget->gSpinBox->value());
    settings.setValue(B, this->toolsTreesTabWidget->bSpinBox->value());
    settings.setValue(A, this->toolsTreesTabWidget->aSpinBox->value());

    settings.setValue(USE_LAST_RADIUS_AS_DEFAULT, this->toolsNodesTabWidget->useLastRadiusBox->isChecked());
    settings.setValue(ACTIVE_NODE_RADIUS, this->toolsNodesTabWidget->activeNodeRadiusSpinBox->value());
    settings.setValue(DEFAULT_NODE_RADIUS, this->toolsNodesTabWidget->defaultNodeRadiusSpinBox->value());
    settings.setValue(ENABLE_COMMENT_LOCKING, this->toolsNodesTabWidget->enableCommentLockingCheckBox->isChecked());
    settings.setValue(LOCKING_RADIUS, this->toolsNodesTabWidget->lockingRadiusSpinBox->value());
    settings.setValue(LOCK_TO_NODES_WITH_COMMENT, this->toolsNodesTabWidget->lockingToNodesWithCommentField->text());

    settings.endGroup();
}

void ToolsWidget::updateToolsSlot() {
    toolsNodesTabWidget->updateToolsNodesTab();
    toolsQuickTabWidget->updateToolsQuickTab();
    toolsTreesTabWidget->updateToolsTreesTab();
}

void ToolsWidget::toggleAllWidgets(bool enabled) {
    this->toolsQuickTabWidget->activeTreeSpinBox->setEnabled(enabled);
    this->toolsQuickTabWidget->activeNodeSpinBox->setEnabled(enabled);

    this->toolsQuickTabWidget->commentField->setEnabled(enabled);
    this->toolsQuickTabWidget->searchForField->setEnabled(enabled);
    this->toolsQuickTabWidget->findNextButton->setEnabled(enabled);
    this->toolsQuickTabWidget->findPreviousButton->setEnabled(enabled);
    this->toolsQuickTabWidget->pushBranchNodeButton->setEnabled(enabled);
    this->toolsQuickTabWidget->popBranchNodeButton->setEnabled(enabled);

    // disable tree tab widgets
    this->toolsTreesTabWidget->activeTreeSpinBox->setEnabled(enabled);

    this->toolsTreesTabWidget->deleteActiveTreeButton->setEnabled(enabled);
    this->toolsTreesTabWidget->commentField->setEnabled(enabled);
    this->toolsTreesTabWidget->mergeTreesButton->setEnabled(enabled);
    this->toolsTreesTabWidget->id1SpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->id2SpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->splitByConnectedComponentsButton->setEnabled(enabled);
    this->toolsTreesTabWidget->rSpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->gSpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->bSpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->aSpinBox->setEnabled(enabled);
    this->toolsTreesTabWidget->restoreDefaultColorButton->setEnabled(enabled);

    // disabled node tab widgets
    this->toolsNodesTabWidget->activeNodeIdSpinBox->setEnabled(enabled);
    this->toolsNodesTabWidget->activeNodeXSpin->setEnabled(enabled);
    this->toolsNodesTabWidget->activeNodeYSpin->setEnabled(enabled);
    this->toolsNodesTabWidget->activeNodeZSpin->setEnabled(enabled);

    this->toolsNodesTabWidget->jumpToNodeButton->setEnabled(enabled);
    this->toolsNodesTabWidget->deleteNodeButton->setEnabled(enabled);
    this->toolsNodesTabWidget->linkNodeWithButton->setEnabled(enabled);
    this->toolsNodesTabWidget->idSpinBox->setEnabled(enabled);
    this->toolsNodesTabWidget->activeNodeRadiusSpinBox->setEnabled(enabled);
    this->toolsNodesTabWidget->commentField->setEnabled(enabled);
    this->toolsNodesTabWidget->searchForField->setEnabled(enabled);
    this->toolsNodesTabWidget->findNextButton->setEnabled(enabled);
    this->toolsNodesTabWidget->findPreviousButton->setEnabled(enabled);
    this->toolsNodesTabWidget->lockToActiveNodeButton->setEnabled(enabled);
    this->toolsNodesTabWidget->disableLockingButton->setEnabled(enabled);
}


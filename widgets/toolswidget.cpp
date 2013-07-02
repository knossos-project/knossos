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

#include "toolswidget.h"
#include "widgets/tools/toolsquicktabwidget.h"
#include "widgets/tools/toolsnodestabwidget.h"
#include "widgets/tools/toolstreestabwidget.h"
#include "GUIConstants.h"
#include "knossos-global.h"
#include <QSettings>

extern stateInfo *state;

ToolsWidget::ToolsWidget(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowTitle("Tools");
    tabs = new QTabWidget(this);

    this->toolsQuickTabWidget = new ToolsQuickTabWidget();
    this->toolsNodesTabWidget = new ToolsNodesTabWidget();
    this->toolsTreesTabWidget = new ToolsTreesTabWidget();

    tabs->addTab(toolsQuickTabWidget, "Quick");
    tabs->addTab(toolsTreesTabWidget, "Trees");
    tabs->addTab(toolsNodesTabWidget, "Nodes");

}

void ToolsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
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
    this->toolsNodesTabWidget->activeNodeRadiusSpinBox->setValue(settings.value(ACTIVE_NODE_RADIUS).toDouble());
    this->toolsNodesTabWidget->defaultNodeRadiusSpinBox->setValue(settings.value(DEFAULT_NODE_RADIUS).toDouble());
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

void ToolsWidget::updateDisplayedTree() {
    this->toolsQuickTabWidget->treeCountLabel->setText(QString("Tree Count: %1").arg(state->skeletonState->treeElements));
    this->toolsQuickTabWidget->activeNodeSpinBox->setMaximum(state->skeletonState->treeElements);
    this->toolsQuickTabWidget->nodeCountLabel->setText(QString("Node Count: %1").arg(state->skeletonState->totalNodeElements));
    this->toolsQuickTabWidget->activeNodeSpinBox->setMaximum(state->skeletonState->totalNodeElements);
    this->toolsQuickTabWidget->activeNodeSpinBox->setValue(state->skeletonState->totalNodeElements);

    this->toolsQuickTabWidget->xLabel->setText(QString("x: %1").arg(state->skeletonState->activeNode->position.x));
    this->toolsQuickTabWidget->yLabel->setText(QString("y: %1").arg(state->skeletonState->activeNode->position.y));
    this->toolsQuickTabWidget->zLabel->setText(QString("z: %3").arg(state->skeletonState->activeNode->position.z));
    this->toolsQuickTabWidget->onStackLabel->setText(QString("on Stack: %1").arg(state->skeletonState->branchStack->elementsOnStack));
    this->toolsQuickTabWidget->update();
}

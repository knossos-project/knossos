#ifndef TOOLSQUICKTABWIDGET_H
#define TOOLSQUICKTABWIDGET_H

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

#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include "knossos-global.h"
#include "../toolswidget.h"
#include "toolsnodestabwidget.h"

class QLabel;
class QPushButton;
class ToolsQuickTabWidget : public QWidget
{
    friend class WidgetContainer;
    friend class ToolsWidget;
    friend class ToolsNodesTabWidget;
    friend class TestCommentsWidget;
    friend class TestToolsWidget;
    Q_OBJECT
public:
    explicit ToolsQuickTabWidget(ToolsWidget *parent = 0);
signals:
    void nextCommentSignal(char *searchString);
    void previousCommentSignal(char *searchString);
    void setActiveTreeSignal(int id);
    bool setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void updateToolsSignal();
    void popBranchNodeSignal(int targetRevision, int serialize);
    void pushBranchNodeSignal(int targetRevision, int setBranchNodeFlag, int checkDoubleBranchpoint, nodeListElement *branchNode, int branchNodeID, int serialize);

    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);
    void updateViewerStateSignal();
    void updateCommentsTableSignal();
    treeListElement *findTreeByTreeIDSignal(int value);
    nodeListElement *findNodeByNodeIDSignal(int value);
    bool addCommentSignal(int targetRevision, const char *content, nodeListElement *node, int nodeID, int serialize);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID, int serialize);
public slots:
    void activeTreeIdChanged(int value);
    void activeNodeIdChanged(int value);
    void commentChanged(QString comment);
    void searchForChanged(QString comment);
    void findNextButtonClicked();
    void findPreviousButtonClicked();
    void pushBranchNodeButtonClicked();
    void popBranchNodeButtonClicked();
public:
    ToolsWidget *reference;
    QLabel *treeCountLabel, *nodeCountLabel;
    QLabel *activeTreeLabel, *activeNodeLabel;
    QLabel *xLabel, *yLabel, *zLabel;
    QSpinBox *activeTreeSpinBox, *activeNodeSpinBox;

    QLabel *commentLabel, *searchForLabel;
    QLineEdit *commentField, *searchForField;
    QPushButton *findNextButton, *findPreviousButton;

    QLabel *branchPointLabel, *onStackLabel;
    QPushButton *pushBranchNodeButton, *popBranchNodeButton;
};

#endif // TOOLSQUICKTABWIDGET_H

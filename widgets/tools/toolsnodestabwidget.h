#ifndef TOOLSNODESTABWIDGET_H
#define TOOLSNODESTABWIDGET_H

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
#include <QCheckBox>
#include <QPushButton>
#include "knossos-global.h"
#include "../toolswidget.h"
#include "toolsquicktabwidget.h"

class QLabel;
class QSpinBox;
class QDoubleSpinBox;
//class QPushButton;
class QLineEdit;

class ToolsNodesTabWidget : public QWidget
{
    friend class WidgetContainer;
    friend class ToolsWidget;
    friend class ToolsQuickTabWidget;
    friend class TestCommentsWidget;
    friend class TestToolsWidget;
    Q_OBJECT
public:
    explicit ToolsNodesTabWidget(ToolsWidget *parent = 0);
    ToolsWidget *reference;
signals:
    void updatePositionSignal(int serverMovement);
    void deleteActiveNodeSignal();
    void lockPositionSignal(Coordinate coordinate);
    void unlockPositionSignal();
    void nextCommentSignal(char *searchString);
    void previousCommentSignal(char *searchString);
    void setActiveNodeSignal(int targetRevision, nodeListElement *node, int nodeID);
    void setRemoteStateTypeSignal(int type);
    void setRecenteringPositionSignal(int x, int y, int z);
    void updateViewerStateSignal();
    void updateCommentsTableSignal();
    nodeListElement *findNodeByNodeIDSignal(int value);
    bool addCommentSignal(int targetRevision, const char *content, nodeListElement *node, int nodeID, int serialize);
    bool editCommentSignal(int targetRevision, commentListElement *currentComment, int nodeID, char *newContent, nodeListElement *newNode, int newNodeID, int serialize);
    bool addSegmentSignal(int targetRevision, int sourceNodeID, int targetNodeID, int serialize);
public slots:
    void idChanged(int value);    
    void jumpToNodeButtonClicked();
    void deleteNodeButtonClicked();
    void linkNodeWithButtonClicked();
    void findNextButtonClicked();
    void findPreviousButtonClicked();
    void useLastRadiusChecked(bool on);
    void activeNodeRadiusChanged(double value);
    void defaultNodeRadiusChanged(double value);
    void enableCommentLockingChecked(bool on);
    void lockingRadiusChanged(int value);
    void lockToNodesWithCommentChanged(QString comment);
    void lockToActiveNodeButtonClicked();
    void disableLockingButtonClicked();

protected:
    QLabel *activeNodeIdLabel, *idLabel;
    QSpinBox *activeNodeIdSpinBox, *idSpinBox;
    QPushButton *jumpToNodeButton, *deleteNodeButton, *linkNodeWithButton;

    QLabel *commentLabel, *searchForLabel;
    QLineEdit *commentField, *searchForField;
    QPushButton *findNextButton, *findPreviousButton;

    QCheckBox *useLastRadiusBox;
    QLabel *activeNodeRadiusLabel, *defaultNodeRadiusLabel;
    QDoubleSpinBox *activeNodeRadiusSpinBox, *defaultNodeRadiusSpinBox;

    QCheckBox *enableCommentLockingCheckBox;
    QLabel *lockingRadiusLabel, *lockToNodesWithCommentLabel;
    QSpinBox *lockingRadiusSpinBox;
    QLineEdit *lockingToNodesWithCommentField;

    QPushButton *lockToActiveNodeButton;
    QPushButton *disableLockingButton;

};

#endif // TOOLSNODESTABWIDGET_H

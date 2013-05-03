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

class QLabel;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QCheckBox;
class ToolsNodesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsNodesTabWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    void updatePositionSignal(int32_t serverMovement);
    void deleteActiveNodeSignal();
public slots:
    void activeNodeChanged(int value);
    void idChanged(int value);
    void commentChanged(QString comment);
    void searchForChanged(QString comment);

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

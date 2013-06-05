#ifndef NAVIGATIONWIDGET_H
#define NAVIGATIONWIDGET_H

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

#include <QDialog>

class QSpinBox;
class QRadioButton;
class QLabel;
class NavigationWidget : public QDialog
{
    Q_OBJECT
public:
    explicit NavigationWidget(QWidget *parent = 0);
    void loadSettings();
    void saveSettings();

signals:
    void uncheckSignal();
public slots:
    void movementSpeedChanged(int value);
    void jumpFramesChanged(int value);
    void walkFramesChanged(int value);
    void recenterTimeParallelChanged(int value);
    void recenterTimeOrthoChanged(int value);
    void normalModeButtonClicked(bool on);
    void additionalViewportDirectionMoveButtonClicked(bool on);
    void additionalTracingDirectionMoveButtonClicked(bool on);
    void additionalMirroredMoveButtonClicked(bool on);
    void delayTimePerStepChanged(int value);
    void numberOfStepsChanged(int value);
protected:
    void closeEvent(QCloseEvent *event);

    QLabel *generalLabel;
    QLabel *movementSpeedLabel;
    QLabel *jumpFramesLabel;
    QLabel *walkFramesLabel;
    QLabel *recenterTimeParallelLabel;
    QLabel *recenterTimeOrthoLabel;
    QLabel *advanceTracingModesLabel;
    QLabel *delayTimePerStepLabel;
    QLabel *numberOfStepsLabel;

    QSpinBox *movementSpeedSpinBox;
    QSpinBox *jumpFramesSpinBox;
    QSpinBox *walkFramesSpinBox;
    QSpinBox *recenterTimeParallelSpinBox;
    QSpinBox *recenterTimeOrthoSpinBox;

    QRadioButton *normalModeButton;
    QRadioButton *additionalViewportDirectionMoveButton;
    QRadioButton *additionalTracingDirectionMoveButton;
    QRadioButton *additionalMirroredMoveButton;
    QSpinBox *delayTimePerStepSpinBox;
    QSpinBox *numberOfStepsSpinBox;

};

#endif // NAVIGATIONWIDGET_H

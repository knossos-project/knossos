#ifndef TOOLSTREESTABWIDGET_H
#define TOOLSTREESTABWIDGET_H

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
class QLineEdit;
class QDoubleSpinBox;
class QPushButton;
class ToolsTreesTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolsTreesTabWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    void delActiveTreeSignal();
public slots:
    void activeTreeIDChanged(int value);
    void commentChanged(QString comment);
    void deleteActiveTreeButtonClicked();
    void newTreeButtonClicked();
    void mergeTreesButtonClicked();
    void id1Changed(int value);
    void id2Changed(int value);
    void splitByConnectedComponentsButtonClicked();
    void rChanged(double value);
    void gChanged(double value);
    void bChanged(double value);
    void aChanged(double value);
    void restoreDefaultColorButtonClicked();


protected:
    QLabel *activeTreeLabel;
    QSpinBox *activeTreeSpinBox;

    QPushButton *deleteActiveTreeButton;
    QPushButton *newTreeButton;

    QLabel *commentLabel;
    QLineEdit *commentField;

    QPushButton *mergeTreesButton, *splitByConnectedComponentsButton, *restoreDefaultColorButton;
    QLabel *id1Label, *id2Label;
    QSpinBox *id1SpinBox, *id2SpinBox;

    QLabel *rLabel, *gLabel, *bLabel, *aLabel;
    QDoubleSpinBox *rSpinBox, *gSpinBox, *bSpinBox, *aSpinBox;

};

#endif // TOOLSTREESTABWIDGET_H

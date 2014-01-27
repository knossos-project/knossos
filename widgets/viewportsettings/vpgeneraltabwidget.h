#ifndef VPGENERALTABWIDGET_H
#define VPGENERALTABWIDGET_H

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
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QRadioButton;
class QPushButton;
class VPGeneralTabWidget : public QWidget
{
    friend class ViewportSettingsWidget;
    Q_OBJECT
public:
    explicit VPGeneralTabWidget(QWidget *parent = 0);
    QPushButton *resetVPsButton;
    QCheckBox *showVPDecorationCheckBox;
signals:
    void overrideNodeRadiusSignal(bool on);
    void highlightActiveTreeSignal(bool on);
    void skeletonChangedSignal(bool on);
    void showNodeID(bool on);
    void segRadiusToNodeRadiusSignal(float value);
public slots:
    void lightEffectsChecked(bool on);
    void hightlightActiveTreeChecked(bool on);
    void showAllNodeIdsChecked(bool on);
    void overrideNodeRadiusChecked(bool on);
    void overrideNodeRadiusChanged(double value);
    void edgeNodeRadiusRatioChanged(double value);
    void renderingQualityChanged(int value);

protected:
    QLabel *skeletonVisualizationLabel;
    QLabel *renderingQualityLabel;
    QSpinBox *renderingQualitySpinBox;
    QCheckBox *lightEffectsCheckBox;
    QCheckBox *hightlightActiveTreeCheckBox;
    QCheckBox *showAllNodeIdsCheckBox;
    QCheckBox *overrideNodeRadiusCheckBox;
    QDoubleSpinBox *overrideNodeRadiusSpinBox;

    QDoubleSpinBox *edgeNodeRadiusRatioSpinBox;
    QLabel *edgeNodeRadiusRatioLabel;
};

#endif // VPGENERALTABWIDGET_H

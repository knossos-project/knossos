#ifndef VPSLICEPLANEVIEWPORTWIDGET_H
#define VPSLICEPLANEVIEWPORTWIDGET_H

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
#include <qgl.h>
class QLabel;
class QCheckBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QSlider;
class VPSlicePlaneViewportWidget : public QWidget
{
    friend class ViewportSettingsWidget;
    Q_OBJECT
public:
    explicit VPSlicePlaneViewportWidget(QWidget *parent = 0);
signals:
    bool loadDataSetColortableSignal(QString path, GLuint *table, int type);
    void showIntersectionsSignal(bool value);
    void updateViewerStateSignal();
    bool loadTreeColorTableSignal(QString path, float *table, int type);
    void treeColorAdjustmentsChangedSignal();
public slots:
    void enableOverlayChecked(bool on);
    void datasetLinearFilteringChecked(bool on);
    void hightlightIntersectionsChecked(bool on);
    void depthCutoffChanged(double value);
    void useOwnDatasetColorsChecked(bool on);
    void useOwnTreeColorsChecked(bool on);
    void useOwnDatasetColorsButtonClicked();
    void useOwnTreeColorButtonClicked();
    void biasSliderMoved(int value);
    void biasChanged(int value);
    void rangeDeltaSliderMoved(int value);
    void rangeDeltaChanged(int value);
    void enableColorOverlayChecked(bool on);
    void drawIntersectionsCrossHairChecked(bool on);
    void showViewPortsSizeChecked(bool on);
    void updateIntersection();


protected:
    QLabel *skeletonOverlayLabel, *voxelFilteringLabel;
    QCheckBox *enableOverlayCheckBox, *highlightIntersectionsCheckBox, *datasetLinearFilteringCheckBox;
    QLabel *depthCutoffLabel;
    QDoubleSpinBox *depthCutoffSpinBox;

    QLabel *colorLookupTablesLabel;
    QCheckBox *useOwnDatasetColorsCheckBox, *useOwnTreeColorsCheckBox;
    QPushButton *useOwnDatasetColorsButton, *useOwnTreeColorButton;

    QLabel *datasetDynamicRangeLabel, *biasLabel, *rangeDeltaLabel;
    QSpinBox *biasSpinBox, *rangeDeltaSpinBox;
    QSlider *biasSlider, *rangeDeltaSlider;

    QLabel *objectIDOverlayLabel, *viewportObjectsLabel;
    QCheckBox *enableColorOverlayCheckBox, *drawIntersectionsCrossHairCheckBox, *showViewPortsSizeCheckBox;
};

#endif // VPSLICEPLANEVIEWPORTWIDGET_H

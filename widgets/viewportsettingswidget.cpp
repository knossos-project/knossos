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

#include "viewportsettingswidget.h"
#include "viewportsettings/vpgeneraltabwidget.h"
#include "viewportsettings/vpsliceplaneviewportwidget.h"
#include "viewportsettings/vpskeletonviewportwidget.h"
#include <QSettings>
#include "GUIConstants.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QRadioButton>

ViewportSettingsWidget::ViewportSettingsWidget(QWidget *parent) :
    QDialog(parent)
{

    this->generalTabWidget = new VPGeneralTabWidget();
    this->slicePlaneViewportWidget = new VPSlicePlaneViewportWidget();
    this->skeletonViewportWidget = new VPSkeletonViewportWidget();

    setWindowTitle("Viewport Settings");
    tabs = new QTabWidget(this);
    tabs->addTab(generalTabWidget, "General");
    tabs->addTab(slicePlaneViewportWidget, "Slice Plane Viewports");
    tabs->addTab(skeletonViewportWidget, "Skeleton Viewport");

}

void ViewportSettingsWidget::closeEvent(QCloseEvent *event) {
    this->hide();
    emit uncheckSignal();
}

void ViewportSettingsWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(VIEWPORT_SETTINGS_WIDGET);
    width = settings.value(WIDTH).toInt();
    height = settings.value(HEIGHT).toInt();
    x = settings.value(POS_X).toInt();
    y = settings.value(POS_Y).toInt();
    visible = settings.value(VISIBLE).toBool();

    this->generalTabWidget->lightEffectsButton->setChecked(settings.value(LIGHT_EFFECTS).toBool());
    this->generalTabWidget->hightlightActiveTreeButton->setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE).toBool());
    this->generalTabWidget->showAllNodeIdsButton->setChecked(settings.value(SHOW_ALL_NODE_ID).toBool());

    this->slicePlaneViewportWidget->enableOverlayCheckBox->setChecked(settings.value(ENABLE_OVERLAY).toBool());
    this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->setChecked(settings.value(HIGHLIGHT_INTERSECTIONS).toBool());
    this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->setChecked(settings.value(DATASET_LINEAR_FILTERING).toBool());
    this->slicePlaneViewportWidget->depthCutoffSpinBox->setValue(settings.value(DEPTH_CUTOFF).toDouble());
    this->slicePlaneViewportWidget->biasSpinBox->setValue(settings.value(BIAS).toInt());
    this->slicePlaneViewportWidget->rangeDeltaSpinBox->setValue(settings.value(RANGE_DELTA).toInt());

    this->slicePlaneViewportWidget->enableColorOverlayCheckBox->setChecked(settings.value(ENABLE_COLOR_OVERLAY).toBool());
    this->slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->setChecked(settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).toBool());
    this->slicePlaneViewportWidget->showViewPortsSizeCheckBox->setChecked(settings.value(SHOW_VIEWPORT_SIZE).toBool());

    this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(settings.value(SHOW_XY_PLANE).toBool());
    this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(settings.value(SHOW_XZ_PLANE).toBool());
    this->skeletonViewportWidget->showYZPlaneCheckBox->setChecked(settings.value(SHOW_YZ_PLANE).toBool());
    this->skeletonViewportWidget->rotateAroundActiveNodeCheckBox->setChecked(settings.value(ROTATE_AROUND_ACTIVE_NODE).toBool());
    this->skeletonViewportWidget->wholeSkeletonButton->setChecked(settings.value(WHOLE_SKELETON).toBool());
    this->skeletonViewportWidget->onlyActiveTreeButton->setChecked(settings.value(ONLY_ACTIVE_TREE).toBool());
    this->skeletonViewportWidget->hideSkeletonButton->setChecked(settings.value(HIDE_SKELETON).toBool());

    settings.endGroup();

    setGeometry(x, y, width, height);

}

void ViewportSettingsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(VIEWPORT_SETTINGS_WIDGET);
    settings.setValue(WIDTH, this->width());
    settings.setValue(HEIGHT, this->height());
    settings.setValue(POS_X, this->x());
    settings.setValue(POS_Y, this->y());
    settings.setValue(VISIBLE, this->isVisible());

    settings.setValue(LIGHT_EFFECTS, this->generalTabWidget->lightEffectsButton->isChecked());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, this->generalTabWidget->hightlightActiveTreeButton->isChecked());
    settings.setValue(SHOW_ALL_NODE_ID, this->generalTabWidget->showAllNodeIdsButton->isChecked());

    settings.setValue(ENABLE_OVERLAY, this->slicePlaneViewportWidget->enableOverlayCheckBox->isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->isChecked());
    settings.setValue(DATASET_LINEAR_FILTERING, this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->isChecked());
    settings.setValue(DEPTH_CUTOFF, this->slicePlaneViewportWidget->depthCutoffSpinBox->value());
    settings.setValue(BIAS, this->slicePlaneViewportWidget->biasSpinBox->value());
    settings.setValue(RANGE_DELTA, this->slicePlaneViewportWidget->rangeDeltaSpinBox->value());
    settings.setValue(ENABLE_COLOR_OVERLAY, this->slicePlaneViewportWidget->enableColorOverlayCheckBox->isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, this->slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->isChecked());
    settings.setValue(SHOW_VIEWPORT_SIZE, this->slicePlaneViewportWidget->showViewPortsSizeCheckBox->isChecked());

    settings.setValue(SHOW_XY_PLANE, this->skeletonViewportWidget->showXYPlaneCheckBox->isChecked());
    settings.setValue(SHOW_XZ_PLANE, this->skeletonViewportWidget->showXZPlaneCheckBox->isChecked());
    settings.setValue(SHOW_YZ_PLANE, this->skeletonViewportWidget->showYZPlaneCheckBox->isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, this->skeletonViewportWidget->rotateAroundActiveNodeCheckBox->isChecked());
    settings.setValue(WHOLE_SKELETON, this->skeletonViewportWidget->wholeSkeletonButton->isChecked());
    settings.setValue(ONLY_ACTIVE_TREE, this->skeletonViewportWidget->onlyActiveTreeButton->isChecked());
    settings.setValue(HIDE_SKELETON, this->skeletonViewportWidget->hideSkeletonButton->isChecked());

    settings.endGroup();
}

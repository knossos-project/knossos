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
#include <QLabel>
#include <QRadioButton>
#include "knossos-global.h"

extern struct stateInfo *state;

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

    if(!settings.value(LIGHT_EFFECTS).isNull()) {
        this->generalTabWidget->lightEffectsCheckBox->setChecked(settings.value(LIGHT_EFFECTS).toBool());
        state->viewerState->lightOnOff = settings.value(LIGHT_EFFECTS).toBool();
    }

    if(!settings.value(HIGHLIGHT_ACTIVE_TREE).isNull()) {
        this->generalTabWidget->hightlightActiveTreeCheckBox->setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE).toBool());
        state->skeletonState->highlightActiveTree = settings.value(HIGHLIGHT_ACTIVE_TREE).toBool();
    }

    if(!settings.value(SHOW_ALL_NODE_ID).isNull()) {
        this->generalTabWidget->showAllNodeIdsCheckBox->setChecked(settings.value(SHOW_ALL_NODE_ID).toBool());
        state->skeletonState->showNodeIDs = settings.value(SHOW_ALL_NODE_ID).toBool();
    }

    if(!settings.value(EDGE_TO_NODE_RADIUS).isNull()) {
        this->generalTabWidget->edgeNodeRadiusRatioSpinBox->setValue(settings.value(EDGE_TO_NODE_RADIUS).toDouble());
        state->skeletonState->segRadiusToNodeRadius = settings.value(EDGE_TO_NODE_RADIUS).toDouble();
    }

    if(!settings.value(RENDERING_QUALITY).isNull()) {
        this->generalTabWidget->renderingQualitySpinBox->setValue(settings.value(RENDERING_QUALITY).toInt());
        state->viewerState->cumDistRenderThres = settings.value(RENDERING_QUALITY).toInt();
    }


    if(!settings.value(OVERRIDE_NODES_RADIUS_CHECKED).isNull()) {
        this->generalTabWidget->overrideNodeRadiusCheckBox->setChecked(settings.value(OVERRIDE_NODES_RADIUS_CHECKED).toBool());
        state->skeletonState->overrideNodeRadiusBool = settings.value(OVERRIDE_NODES_RADIUS_CHECKED).toBool();
    }

    if(!settings.value(OVERRIDE_NODES_RADIUS_VALUE).isNull()) {
        this->generalTabWidget->overrideNodeRadiusSpinBox->setValue(settings.value(OVERRIDE_NODES_RADIUS_VALUE).toDouble());
        state->skeletonState->overrideNodeRadiusVal = settings.value(OVERRIDE_NODES_RADIUS_VALUE).toDouble();
    } else {
        this->generalTabWidget->overrideNodeRadiusSpinBox->setValue(false);
        state->skeletonState->overrideNodeRadiusVal = 0;
        this->generalTabWidget->edgeNodeRadiusRatioLabel->setStyleSheet("color: gray");
    }


    if(!settings.value(SHOW_POS_AND_SIZE_CHECKED).isNull()) {
        this->generalTabWidget->showPosAndResizeCheckBox->setChecked(settings.value(SHOW_POS_AND_SIZE_CHECKED).toBool());
        // @todo the state->xy is missing
    }

    if(!settings.value(ENABLE_OVERLAY).isNull()) {
        this->slicePlaneViewportWidget->enableOverlayCheckBox->setChecked(settings.value(ENABLE_OVERLAY).toBool());
    } else {
        this->slicePlaneViewportWidget->enableOverlayCheckBox->setChecked(true);
        state->overlay = true;
    }

    if(!settings.value(HIGHLIGHT_INTERSECTIONS).isNull()) {
        this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->setChecked(settings.value(HIGHLIGHT_INTERSECTIONS).toBool());
        state->skeletonState->showIntersections = settings.value(HIGHLIGHT_INTERSECTIONS).toBool();
    }


    if(!settings.value(DATASET_LINEAR_FILTERING).isNull()) {
        this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->setChecked(settings.value(DATASET_LINEAR_FILTERING).toBool());
        bool on = settings.value(DATASET_LINEAR_FILTERING).toBool();
        if(on)
            state->viewerState->filterType = GL_LINEAR;
        else
            state->viewerState->filterType = GL_NEAREST;
    }

    if(!settings.value(DEPTH_CUTOFF).isNull()) {
        this->slicePlaneViewportWidget->depthCutoffSpinBox->setValue(settings.value(DEPTH_CUTOFF).toDouble());
        state->viewerState->depthCutOff = settings.value(DEPTH_CUTOFF).toDouble();
    } else {
        this->slicePlaneViewportWidget->depthCutoffSpinBox->setValue(1);
        state->viewerState->depthCutOff = 1;
    }


    if(!settings.value(BIAS).isNull()) {
        this->slicePlaneViewportWidget->biasSpinBox->setValue(settings.value(BIAS).toInt());
        state->viewerState->luminanceBias = settings.value(BIAS).toInt();
    } else {
        this->slicePlaneViewportWidget->biasSpinBox->setValue(0);
        state->viewerState->luminanceBias = 0;
    }

    if(!settings.value(RANGE_DELTA).isNull()) {
        this->slicePlaneViewportWidget->rangeDeltaSpinBox->setValue(settings.value(RANGE_DELTA).toInt());
        state->viewerState->luminanceRangeDelta = settings.value(RANGE_DELTA).toInt();
    } else {
        this->slicePlaneViewportWidget->rangeDeltaSpinBox->setValue(this->slicePlaneViewportWidget->rangeDeltaSpinBox->maximum());
        state->viewerState->luminanceRangeDelta = 255;
    }


    if(!settings.value(ENABLE_COLOR_OVERLAY).isNull()) {
        this->slicePlaneViewportWidget->enableColorOverlayCheckBox->setChecked(settings.value(ENABLE_COLOR_OVERLAY).toBool());
        state->viewerState->overlayVisible = settings.value(ENABLE_COLOR_OVERLAY).toBool();
    }

    if(!settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).isNull()) {
        this->slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->setChecked(settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).toBool());
        state->viewerState->drawVPCrosshairs = settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).toBool();
    }

    if(!settings.value(SHOW_VIEWPORT_SIZE).isNull()) {
        this->slicePlaneViewportWidget->showViewPortsSizeCheckBox->setChecked(settings.value(SHOW_VIEWPORT_SIZE).toBool());
        state->viewerState->showVPLabels = settings.value(SHOW_VIEWPORT_SIZE).toBool();
    }

    if(!settings.value(SHOW_XY_PLANE).isNull()) {
        this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(settings.value(SHOW_XY_PLANE).toBool());
        state->skeletonState->showXYplane = settings.value(SHOW_XY_PLANE).toBool();
    }

    if(!settings.value(SHOW_XZ_PLANE).isNull()) {
        this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(settings.value(SHOW_XZ_PLANE).toBool());
        state->skeletonState->showXZplane = settings.value(SHOW_XZ_PLANE).toBool();
    }

    if(!settings.value(SHOW_YZ_PLANE).isNull()) {
        this->skeletonViewportWidget->showYZPlaneCheckBox->setChecked(settings.value(SHOW_YZ_PLANE).toBool());
        state->skeletonState->showYZplane = settings.value(SHOW_YZ_PLANE).toBool();
    }
    if(!settings.value(ROTATE_AROUND_ACTIVE_NODE).isNull()) {
        this->skeletonViewportWidget->rotateAroundActiveNodeCheckBox->setChecked(settings.value(ROTATE_AROUND_ACTIVE_NODE).toBool());
        state->skeletonState->rotateAroundActiveNode = settings.value(ROTATE_AROUND_ACTIVE_NODE).toBool();
    }

    if(!settings.value(WHOLE_SKELETON).isNull()) {
        this->skeletonViewportWidget->wholeSkeletonRadioButton->setChecked(settings.value(WHOLE_SKELETON).toBool());
        bool on = settings.value(WHOLE_SKELETON).toBool();
        if(!on) {
            state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
        } else {
           state->skeletonState->displayMode &= ~DSP_SKEL_VP_WHOLE;
        }
    }

    if(!settings.value(ONLY_ACTIVE_TREE).isNull()) {
        this->skeletonViewportWidget->onlyActiveTreeRadioButton->setChecked(settings.value(ONLY_ACTIVE_TREE).toBool());
        bool on = settings.value(ONLY_ACTIVE_TREE).toBool();
        if(on) {          
            state->skeletonState->displayMode |= DSP_ACTIVETREE;
        } else {
            state->skeletonState->displayMode &= ~DSP_ACTIVETREE;
        }
    }

    if(!settings.value(HIDE_SKELETON).isNull()) {
        this->skeletonViewportWidget->hideSkeletonRadioButton->setChecked(settings.value(HIDE_SKELETON).toBool());
        bool on = settings.value(HIDE_SKELETON).toBool();
        if(on) {            
            state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
        } else {
            state->skeletonState->displayMode &= ~DSP_SKEL_VP_HIDE;
        }
    }


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

    settings.setValue(LIGHT_EFFECTS, this->generalTabWidget->lightEffectsCheckBox->isChecked());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, this->generalTabWidget->hightlightActiveTreeCheckBox->isChecked());
    settings.setValue(SHOW_ALL_NODE_ID, this->generalTabWidget->showAllNodeIdsCheckBox->isChecked());
    settings.setValue(EDGE_TO_NODE_RADIUS, this->generalTabWidget->edgeNodeRadiusRatioSpinBox->value());
    settings.setValue(RENDERING_QUALITY, this->generalTabWidget->renderingQualitySpinBox->value());
    settings.setValue(OVERRIDE_NODES_RADIUS_CHECKED, this->generalTabWidget->overrideNodeRadiusCheckBox->isChecked());
    settings.setValue(OVERRIDE_NODES_RADIUS_VALUE, this->generalTabWidget->overrideNodeRadiusSpinBox->value());
    settings.setValue(SHOW_POS_AND_SIZE_CHECKED, this->generalTabWidget->showPosAndResizeCheckBox->isChecked());

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
    settings.setValue(WHOLE_SKELETON, this->skeletonViewportWidget->wholeSkeletonRadioButton->isChecked());
    settings.setValue(ONLY_ACTIVE_TREE, this->skeletonViewportWidget->onlyActiveTreeRadioButton->isChecked());
    settings.setValue(HIDE_SKELETON, this->skeletonViewportWidget->hideSkeletonRadioButton->isChecked());

    settings.endGroup();
}

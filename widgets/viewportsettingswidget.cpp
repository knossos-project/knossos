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
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include "knossos-global.h"

extern stateInfo *state;

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

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tabs);
    setLayout(layout);
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
    width = (settings.value(WIDTH).isNull())? this->width() : settings.value(WIDTH).toInt();
    height = (settings.value(HEIGHT).isNull())? this->height() : settings.value(HEIGHT).toInt();
    if(settings.value(POS_X).isNull() or settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + this->height();
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = (settings.value(VISIBLE).isNull())? false : settings.value(VISIBLE).toBool();

    state->viewerState->lightOnOff =
            (settings.value(LIGHT_EFFECTS).isNull())? true : settings.value(LIGHT_EFFECTS).toBool();
    this->generalTabWidget->lightEffectsCheckBox->setChecked(state->viewerState->lightOnOff);

    state->skeletonState->highlightActiveTree =
            (settings.value(HIGHLIGHT_ACTIVE_TREE).isNull())? true : settings.value(HIGHLIGHT_ACTIVE_TREE).toBool();
    this->generalTabWidget->hightlightActiveTreeCheckBox->setChecked(state->skeletonState->highlightActiveTree);

    state->skeletonState->showNodeIDs =
            (settings.value(SHOW_ALL_NODE_ID).isNull())? false : settings.value(SHOW_ALL_NODE_ID).toBool();
    this->generalTabWidget->showAllNodeIdsCheckBox->setChecked(state->skeletonState->showNodeIDs);

    state->skeletonState->segRadiusToNodeRadius =
            (settings.value(EDGE_TO_NODE_RADIUS).isNull())? 1.5 : settings.value(EDGE_TO_NODE_RADIUS).toDouble();
    this->generalTabWidget->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);

    state->viewerState->cumDistRenderThres =
            (settings.value(RENDERING_QUALITY).isNull())? 7. : settings.value(RENDERING_QUALITY).toInt();
    this->generalTabWidget->renderingQualitySpinBox->setValue((int)state->viewerState->cumDistRenderThres);

    state->skeletonState->overrideNodeRadiusBool =
            (settings.value(OVERRIDE_NODES_RADIUS_CHECKED).isNull())? false : settings.value(OVERRIDE_NODES_RADIUS_CHECKED).toBool();
    this->generalTabWidget->overrideNodeRadiusCheckBox->setChecked(state->skeletonState->overrideNodeRadiusBool);

    if(settings.value(OVERRIDE_NODES_RADIUS_VALUE).isNull() == false) {
        state->skeletonState->overrideNodeRadiusVal = settings.value(OVERRIDE_NODES_RADIUS_VALUE).toDouble();
    } else {
        state->skeletonState->overrideNodeRadiusVal = 1.5;
    }
    this->generalTabWidget->overrideNodeRadiusSpinBox->setValue(state->skeletonState->overrideNodeRadiusVal);
    this->generalTabWidget->overrideNodeRadiusSpinBox->setEnabled(state->skeletonState->overrideNodeRadiusBool);

    state->skeletonState->segRadiusToNodeRadius =
            (settings.value(EDGE_TO_NODE_RADIUS).isNull())? 0.5 : settings.value(EDGE_TO_NODE_RADIUS).toFloat();
    this->generalTabWidget->edgeNodeRadiusRatioSpinBox->setValue(state->skeletonState->segRadiusToNodeRadius);

    if(settings.value(SHOW_VP_DECORATION).isNull() == false) {
        this->generalTabWidget->showVPDecorationCheckBox->setChecked(settings.value(SHOW_VP_DECORATION).toBool());
    }
    else {
        this->generalTabWidget->showVPDecorationCheckBox->setChecked(false);
    }

    state->overlay = (settings.value(ENABLE_OVERLAY).isNull())? true : settings.value(ENABLE_OVERLAY).toBool();
    this->slicePlaneViewportWidget->enableOverlayCheckBox->setChecked(state->overlay);

    state->skeletonState->showIntersections =
            (settings.value(HIGHLIGHT_INTERSECTIONS).isNull())? false : settings.value(HIGHLIGHT_INTERSECTIONS).toBool();
    this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->setChecked(state->skeletonState->showIntersections);


    state->viewerState->filterType = GL_NEAREST;
    if(settings.value(DATASET_LINEAR_FILTERING).isNull() == false) {
        if(settings.value(DATASET_LINEAR_FILTERING).toBool()) {
            state->viewerState->filterType = GL_LINEAR;
        }
    }
    this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->setChecked(state->viewerState->filterType);

    state->viewerState->depthCutOff = (settings.value(DEPTH_CUTOFF).isNull())? 5. : settings.value(DEPTH_CUTOFF).toDouble();
    this->slicePlaneViewportWidget->depthCutoffSpinBox->setValue(state->viewerState->depthCutOff);

    state->viewerState->luminanceBias = (settings.value(BIAS).isNull())? 0 : settings.value(BIAS).toInt();
    this->slicePlaneViewportWidget->biasSpinBox->setValue(state->viewerState->luminanceBias);

    state->viewerState->luminanceRangeDelta = (settings.value(RANGE_DELTA).isNull())? 255 : settings.value(RANGE_DELTA).toInt();
    this->slicePlaneViewportWidget->rangeDeltaSpinBox->setValue(state->viewerState->luminanceRangeDelta);

    state->viewerState->overlayVisible =
            (settings.value(ENABLE_COLOR_OVERLAY).isNull())? false : settings.value(ENABLE_COLOR_OVERLAY).toBool();
    this->slicePlaneViewportWidget->enableColorOverlayCheckBox->setChecked(state->viewerState->overlayVisible);

    state->viewerState->drawVPCrosshairs =
            (settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).isNull())? true : settings.value(DRAW_INTERSECTIONS_CROSSHAIRS).toBool();
    this->slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->setChecked(state->viewerState->drawVPCrosshairs);

    state->viewerState->showVPLabels =
            (settings.value(SHOW_VIEWPORT_SIZE).isNull())? false : settings.value(SHOW_VIEWPORT_SIZE).toBool();
    this->slicePlaneViewportWidget->showViewPortsSizeCheckBox->setChecked(state->viewerState->showVPLabels);

    state->skeletonState->showXYplane =
            (settings.value(SHOW_XY_PLANE).isNull())? false : settings.value(SHOW_XY_PLANE).toBool();
    this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(state->skeletonState->showXYplane);

    state->skeletonState->showXZplane =
            (settings.value(SHOW_XZ_PLANE).isNull())? false : settings.value(SHOW_XZ_PLANE).toBool();
    this->skeletonViewportWidget->showXYPlaneCheckBox->setChecked(state->skeletonState->showXZplane);

    state->skeletonState->showYZplane =
            (settings.value(SHOW_YZ_PLANE).isNull())? false : settings.value(SHOW_YZ_PLANE).toBool();
    this->skeletonViewportWidget->showYZPlaneCheckBox->setChecked(state->skeletonState->showYZplane);

    state->skeletonState->rotateAroundActiveNode =
            (settings.value(ROTATE_AROUND_ACTIVE_NODE).isNull())? true : settings.value(ROTATE_AROUND_ACTIVE_NODE).toBool();
    this->skeletonViewportWidget->rotateAroundActiveNodeCheckBox->setChecked(state->skeletonState->rotateAroundActiveNode);

    state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
    if(settings.value(WHOLE_SKELETON).isNull() == false) {
        if(settings.value(WHOLE_SKELETON).toBool() == false) {
            state->skeletonState->displayMode &= ~DSP_SKEL_VP_WHOLE;
        }
    }
    this->skeletonViewportWidget->wholeSkeletonRadioButton->setChecked(state->skeletonState->displayMode & DSP_SKEL_VP_WHOLE);


    if(!settings.value(ONLY_ACTIVE_TREE).isNull()) {
        this->skeletonViewportWidget->onlyActiveTreeRadioButton->setChecked(settings.value(ONLY_ACTIVE_TREE).toBool());
        bool on = settings.value(ONLY_ACTIVE_TREE).toBool();
        if(on) {          
            state->skeletonState->displayMode |= DSP_ACTIVETREE;
        } else {
            state->skeletonState->displayMode &= ~DSP_ACTIVETREE;
        }
    }

    state->skeletonState->displayMode &= ~DSP_SKEL_VP_HIDE;
    if(settings.value(HIDE_SKELETON).isNull() == false) {
        if(settings.value(HIDE_SKELETON).toBool()) {
            state->skeletonState->displayMode |= DSP_SKEL_VP_HIDE;
        }
    }
    this->skeletonViewportWidget->hideSkeletonRadioButton->setChecked(state->skeletonState->displayMode & DSP_SKEL_VP_HIDE);

    if(settings.value(DATASET_LUT_FILE).isNull() == false) {
        this->slicePlaneViewportWidget->datasetLutFile->setText(settings.value(DATASET_LUT_FILE).toString());
        this->slicePlaneViewportWidget->loadDatasetLUT();
        //this->slicePlaneViewportWidget->useOwnDatasetColorsChecked(true);
    }

    if(!settings.value(TREE_LUT_FILE).isNull()) {
        this->slicePlaneViewportWidget->treeLutFile->setText(settings.value(TREE_LUT_FILE).toString());
        this->slicePlaneViewportWidget->loadTreeLUT();
        //this->slicePlaneViewportWidget->useOwnTreeColorsChecked(true);
    }

    if(this->skeletonViewportWidget->wholeSkeletonRadioButton->isChecked() == false
        and this->skeletonViewportWidget->onlyActiveTreeRadioButton->isChecked() == false
        and this->skeletonViewportWidget->hideSkeletonRadioButton->isChecked() == false) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
        this->skeletonViewportWidget->wholeSkeletonRadioButton->setChecked(true);
    }
    settings.endGroup();
    if(visible) {
        show();
    }
    else {
        hide();
    }
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
    settings.setValue(SHOW_VP_DECORATION, this->generalTabWidget->showVPDecorationCheckBox->isChecked());

    settings.setValue(ENABLE_OVERLAY, this->slicePlaneViewportWidget->enableOverlayCheckBox->isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->isChecked());
    settings.setValue(DATASET_LINEAR_FILTERING, this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->isChecked());
    settings.setValue(DEPTH_CUTOFF, this->slicePlaneViewportWidget->depthCutoffSpinBox->value());
    settings.setValue(BIAS, this->slicePlaneViewportWidget->biasSpinBox->value());
    settings.setValue(RANGE_DELTA, this->slicePlaneViewportWidget->rangeDeltaSpinBox->value());
    settings.setValue(ENABLE_COLOR_OVERLAY, this->slicePlaneViewportWidget->enableColorOverlayCheckBox->isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, this->slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->isChecked());
    settings.setValue(SHOW_VIEWPORT_SIZE, this->slicePlaneViewportWidget->showViewPortsSizeCheckBox->isChecked());
    settings.setValue(DATASET_LUT_FILE, this->slicePlaneViewportWidget->datasetLutFile->text());
    settings.setValue(TREE_LUT_FILE, this->slicePlaneViewportWidget->treeLutFile->text());

    qDebug() << this->slicePlaneViewportWidget->datasetLutFile->text();

    if(!this->slicePlaneViewportWidget->datasetLutFile->text().isEmpty()) {
        settings.setValue(DATASET_LUT_FILE, this->slicePlaneViewportWidget->datasetLutFile->text());
        strcpy(state->viewerState->gui->datasetLUTFile, this->slicePlaneViewportWidget->datasetLutFile->text().toStdString().c_str());
    } else {

    }

    if(!this->slicePlaneViewportWidget->treeLutFile->text().isEmpty()) {
        settings.setValue(TREE_LUT_FILE, this->slicePlaneViewportWidget->treeLutFile->text());
        strcpy(state->viewerState->gui->treeLUTFile, this->slicePlaneViewportWidget->treeLutFile->text().toStdString().c_str());
    } else {

    }

    settings.setValue(SHOW_XY_PLANE, this->skeletonViewportWidget->showXYPlaneCheckBox->isChecked());
    settings.setValue(SHOW_XZ_PLANE, this->skeletonViewportWidget->showXZPlaneCheckBox->isChecked());
    settings.setValue(SHOW_YZ_PLANE, this->skeletonViewportWidget->showYZPlaneCheckBox->isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, this->skeletonViewportWidget->rotateAroundActiveNodeCheckBox->isChecked());
    settings.setValue(WHOLE_SKELETON, this->skeletonViewportWidget->wholeSkeletonRadioButton->isChecked());
    settings.setValue(ONLY_ACTIVE_TREE, this->skeletonViewportWidget->onlyActiveTreeRadioButton->isChecked());
    settings.setValue(HIDE_SKELETON, this->skeletonViewportWidget->hideSkeletonRadioButton->isChecked());


    settings.endGroup();


}

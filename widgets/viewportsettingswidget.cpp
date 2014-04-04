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
#include "GuiConstants.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include "knossos-global.h"

extern stateInfo *state;

ViewportSettingsWidget::ViewportSettingsWidget(QWidget *parent) : QDialog(parent) {
    this->generalTabWidget = new VPGeneralTabWidget();
    this->slicePlaneViewportWidget = new VPSlicePlaneViewportWidget();
    this->skeletonViewportWidget = new VPSkeletonViewportWidget();

    setWindowIcon(QIcon(":/images/icons/view-list-icons-symbolic.png"));
    setWindowTitle("Viewport Settings");
    tabs = new QTabWidget(this);
    tabs->addTab(generalTabWidget, "General");
    tabs->addTab(slicePlaneViewportWidget, "Slice Plane Viewports");
    tabs->addTab(skeletonViewportWidget, "Skeleton Viewport");

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tabs);
    setLayout(layout);

    this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void ViewportSettingsWidget::closeEvent(QCloseEvent *) {
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
        emit decorationSignal();
    }
    else {
        this->generalTabWidget->showVPDecorationCheckBox->setChecked(true);
    }

    const auto skeletonOverlay = settings.value(ENABLE_SKELETON_OVERLAY, true).toBool();
    slicePlaneViewportWidget->enableSkeletonOverlayCheckBox->setChecked(skeletonOverlay);
    slicePlaneViewportWidget->enableSkeletonOverlayCheckBox->clicked(skeletonOverlay);

    state->skeletonState->showIntersections =
            (settings.value(HIGHLIGHT_INTERSECTIONS).isNull())? false : settings.value(HIGHLIGHT_INTERSECTIONS).toBool();
    this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->setChecked(state->skeletonState->showIntersections);


    state->viewerState->filterType = GL_LINEAR;
    if(settings.value(DATASET_LINEAR_FILTERING).isNull() == false) {
        if(settings.value(DATASET_LINEAR_FILTERING).toBool() == false) {
            state->viewerState->filterType = GL_NEAREST;
        }
    }
    this->slicePlaneViewportWidget->datasetLinearFilteringCheckBox->setChecked((state->viewerState->filterType == GL_LINEAR)? true : false);

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
    this->skeletonViewportWidget->showXZPlaneCheckBox->setChecked(state->skeletonState->showXZplane);

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

    slicePlaneViewportWidget->datasetLutFile->setText(settings.value(DATASET_LUT_FILE, "").toString());
    if (!slicePlaneViewportWidget->datasetLutFile->text().isEmpty()) {
        slicePlaneViewportWidget->loadDatasetLUT();
    }
    //it’s impotant to populate the checkbox after loading the path-string, because emitted signals depend on the lut
    slicePlaneViewportWidget->useOwnDatasetColorsCheckBox->setChecked(settings.value(DATASET_LUT_FILE_USED, false).toBool());

    slicePlaneViewportWidget->treeLutFile->setText(settings.value(TREE_LUT_FILE, "").toString());
    if (!slicePlaneViewportWidget->treeLutFile->text().isEmpty()) {
        slicePlaneViewportWidget->loadTreeLUT();
    }
    //the same applies here
    slicePlaneViewportWidget->useOwnTreeColorsCheckBox->setChecked(settings.value(TREE_LUT_FILE_USED, false).toBool());

    if(this->skeletonViewportWidget->wholeSkeletonRadioButton->isChecked() == false
        and this->skeletonViewportWidget->onlyActiveTreeRadioButton->isChecked() == false
        and this->skeletonViewportWidget->hideSkeletonRadioButton->isChecked() == false) {
        state->skeletonState->displayMode |= DSP_SKEL_VP_WHOLE;
        this->skeletonViewportWidget->wholeSkeletonRadioButton->setChecked(true);
    }

    tabs->setCurrentIndex(settings.value(VP_TAB_INDEX, 0).toInt());

    settings.endGroup();
    if(visible) {
        show();
    } else {
        hide();
    }
    setGeometry(x, y, width, height);
}

void ViewportSettingsWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(VIEWPORT_SETTINGS_WIDGET);
    settings.setValue(WIDTH, geometry().width());
    settings.setValue(HEIGHT, geometry().height());
    settings.setValue(POS_X, geometry().x());
    settings.setValue(POS_Y, geometry().y());
    settings.setValue(VISIBLE, isVisible());

    settings.setValue(LIGHT_EFFECTS, generalTabWidget->lightEffectsCheckBox->isChecked());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, generalTabWidget->hightlightActiveTreeCheckBox->isChecked());
    settings.setValue(SHOW_ALL_NODE_ID, generalTabWidget->showAllNodeIdsCheckBox->isChecked());
    settings.setValue(EDGE_TO_NODE_RADIUS, generalTabWidget->edgeNodeRadiusRatioSpinBox->value());
    settings.setValue(RENDERING_QUALITY, generalTabWidget->renderingQualitySpinBox->value());
    settings.setValue(OVERRIDE_NODES_RADIUS_CHECKED, generalTabWidget->overrideNodeRadiusCheckBox->isChecked());
    settings.setValue(OVERRIDE_NODES_RADIUS_VALUE, generalTabWidget->overrideNodeRadiusSpinBox->value());
    settings.setValue(SHOW_VP_DECORATION, generalTabWidget->showVPDecorationCheckBox->isChecked());

    settings.setValue(ENABLE_SKELETON_OVERLAY, slicePlaneViewportWidget->enableSkeletonOverlayCheckBox->isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, slicePlaneViewportWidget->highlightIntersectionsCheckBox->isChecked());
    settings.setValue(DATASET_LINEAR_FILTERING, slicePlaneViewportWidget->datasetLinearFilteringCheckBox->isChecked());
    settings.setValue(DEPTH_CUTOFF, slicePlaneViewportWidget->depthCutoffSpinBox->value());
    settings.setValue(BIAS, slicePlaneViewportWidget->biasSpinBox->value());
    settings.setValue(RANGE_DELTA, slicePlaneViewportWidget->rangeDeltaSpinBox->value());
    settings.setValue(ENABLE_COLOR_OVERLAY, slicePlaneViewportWidget->enableColorOverlayCheckBox->isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->isChecked());
    settings.setValue(SHOW_VIEWPORT_SIZE, slicePlaneViewportWidget->showViewPortsSizeCheckBox->isChecked());
    settings.setValue(DATASET_LUT_FILE, slicePlaneViewportWidget->datasetLutFile->text());
    settings.setValue(DATASET_LUT_FILE_USED, slicePlaneViewportWidget->useOwnDatasetColorsCheckBox->isChecked());
    settings.setValue(TREE_LUT_FILE, slicePlaneViewportWidget->treeLutFile->text());
    settings.setValue(TREE_LUT_FILE_USED, slicePlaneViewportWidget->useOwnTreeColorsCheckBox->isChecked());

    settings.setValue(WHOLE_SKELETON, skeletonViewportWidget->wholeSkeletonRadioButton->isChecked());
    settings.setValue(ONLY_ACTIVE_TREE, skeletonViewportWidget->onlyActiveTreeRadioButton->isChecked());
    settings.setValue(HIDE_SKELETON, skeletonViewportWidget->hideSkeletonRadioButton->isChecked());
    settings.setValue(SHOW_XY_PLANE, skeletonViewportWidget->showXYPlaneCheckBox->isChecked());
    settings.setValue(SHOW_XZ_PLANE, skeletonViewportWidget->showXZPlaneCheckBox->isChecked());
    settings.setValue(SHOW_YZ_PLANE, skeletonViewportWidget->showYZPlaneCheckBox->isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, skeletonViewportWidget->rotateAroundActiveNodeCheckBox->isChecked());

    settings.setValue(VP_TAB_INDEX, tabs->currentIndex());

    settings.endGroup();


}

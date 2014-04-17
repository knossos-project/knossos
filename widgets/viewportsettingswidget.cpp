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

#include "knossos-global.h"

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
        this->generalTabWidget->showVPDecorationCheckBox->setChecked(true);
    }


    state->skeletonState->displayMode = 0;//it needs to get initialized _somewhere_
    //sp vp settings
    const auto skeletonOverlay = settings.value(ENABLE_SKELETON_OVERLAY, true).toBool();
    slicePlaneViewportWidget->enableSkeletonOverlayCheckBox->setChecked(skeletonOverlay);
    slicePlaneViewportWidget->enableSkeletonOverlayCheckBox->clicked(skeletonOverlay);

    const auto intersections = settings.value(HIGHLIGHT_INTERSECTIONS, false).toBool();
    this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->setChecked(intersections);
    this->slicePlaneViewportWidget->highlightIntersectionsCheckBox->clicked(intersections);

    const auto linearFiltering = settings.value(DATASET_LINEAR_FILTERING, true).toBool();
    slicePlaneViewportWidget->datasetLinearFilteringCheckBox->setChecked(linearFiltering);
    slicePlaneViewportWidget->datasetLinearFilteringCheckBox->clicked(linearFiltering);

    const auto depthCutoff = settings.value(DEPTH_CUTOFF, 5.).toDouble();
    slicePlaneViewportWidget->depthCutoffSpinBox->setValue(depthCutoff);
    slicePlaneViewportWidget->depthCutoffSpinBox->valueChanged(depthCutoff);

    slicePlaneViewportWidget->datasetLutFile->setText(settings.value(DATASET_LUT_FILE, "").toString());
    if (!slicePlaneViewportWidget->datasetLutFile->text().isEmpty()) {
        slicePlaneViewportWidget->loadDatasetLUT();
    }
    //it’s impotant to populate the checkbox after loading the path-string, because emitted signals depend on the lut
    const auto useDatasetLut = settings.value(DATASET_LUT_FILE_USED, false).toBool();
    slicePlaneViewportWidget->useOwnDatasetColorsCheckBox->setChecked(useDatasetLut);
    slicePlaneViewportWidget->useOwnDatasetColorsCheckBox->clicked(useDatasetLut);

    slicePlaneViewportWidget->treeLutFile->setText(settings.value(TREE_LUT_FILE, "").toString());
    if (!slicePlaneViewportWidget->treeLutFile->text().isEmpty()) {
        slicePlaneViewportWidget->loadTreeLUT();
    }
    //the same applies here
    const auto useTreeLut = settings.value(TREE_LUT_FILE_USED, false).toBool();
    slicePlaneViewportWidget->useOwnTreeColorsCheckBox->setChecked(useTreeLut);
    slicePlaneViewportWidget->useOwnTreeColorsCheckBox->clicked(useTreeLut);

    const auto luminanceBias = settings.value(BIAS, 0).toInt();
    slicePlaneViewportWidget->biasSpinBox->setValue(luminanceBias);
    slicePlaneViewportWidget->biasSpinBox->valueChanged(luminanceBias);

    const auto luminanceRangeDelta = settings.value(RANGE_DELTA, 255).toInt();
    slicePlaneViewportWidget->rangeDeltaSpinBox->setValue(luminanceRangeDelta);
    slicePlaneViewportWidget->rangeDeltaSpinBox->valueChanged(luminanceRangeDelta);

    const auto colorOverlay = settings.value(ENABLE_COLOR_OVERLAY, false).toBool();
    slicePlaneViewportWidget->enableColorOverlayCheckBox->setChecked(colorOverlay);
    slicePlaneViewportWidget->enableColorOverlayCheckBox->clicked(colorOverlay);

    const auto drawVPCrosshairs = settings.value(DRAW_INTERSECTIONS_CROSSHAIRS, true).toBool();
    slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->setChecked(drawVPCrosshairs);
    slicePlaneViewportWidget->drawIntersectionsCrossHairCheckBox->clicked(drawVPCrosshairs);

    const auto showVPLabels = settings.value(SHOW_VIEWPORT_SIZE, false).toBool();
    slicePlaneViewportWidget->showViewPortsSizeCheckBox->setChecked(showVPLabels);
    slicePlaneViewportWidget->showViewPortsSizeCheckBox->clicked(showVPLabels);


    //skeleton vp settings
    const auto wholeSkeleton = settings.value(WHOLE_SKELETON, true).toBool();
    skeletonViewportWidget->wholeSkeletonRadioButton.setChecked(wholeSkeleton);
    skeletonViewportWidget->wholeSkeletonRadioButton.clicked(wholeSkeleton);

    const auto onlyActiveTree = settings.value(ONLY_ACTIVE_TREE, false).toBool();
    skeletonViewportWidget->onlyActiveTreeRadioButton.setChecked(onlyActiveTree);
    skeletonViewportWidget->onlyActiveTreeRadioButton.clicked(onlyActiveTree);

    const auto hideSkeleton = settings.value(HIDE_SKELETON, false).toBool();
    skeletonViewportWidget->hideSkeletonRadioButton.setChecked(hideSkeleton);
    skeletonViewportWidget->hideSkeletonRadioButton.clicked(hideSkeleton);

    const auto xyplane = settings.value(SHOW_XY_PLANE, false).toBool();
    skeletonViewportWidget->showXYPlaneCheckBox.setChecked(xyplane);
    skeletonViewportWidget->showXYPlaneCheckBox.clicked(xyplane);

    const auto xzplane = settings.value(SHOW_XZ_PLANE, false).toBool();
    skeletonViewportWidget->showXZPlaneCheckBox.setChecked(xzplane);
    skeletonViewportWidget->showXZPlaneCheckBox.clicked(xzplane);

    const auto yzplane = settings.value(SHOW_YZ_PLANE, false).toBool();
    skeletonViewportWidget->showYZPlaneCheckBox.setChecked(yzplane);
    skeletonViewportWidget->showYZPlaneCheckBox.clicked(yzplane);

    const auto rotateAroundActiveNode = settings.value(ROTATE_AROUND_ACTIVE_NODE, true).toBool();
    skeletonViewportWidget->rotateAroundActiveNodeCheckBox.setChecked(rotateAroundActiveNode);
    skeletonViewportWidget->rotateAroundActiveNodeCheckBox.clicked(rotateAroundActiveNode);


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

    settings.setValue(WHOLE_SKELETON, skeletonViewportWidget->wholeSkeletonRadioButton.isChecked());
    settings.setValue(ONLY_ACTIVE_TREE, skeletonViewportWidget->onlyActiveTreeRadioButton.isChecked());
    settings.setValue(HIDE_SKELETON, skeletonViewportWidget->hideSkeletonRadioButton.isChecked());
    settings.setValue(SHOW_XY_PLANE, skeletonViewportWidget->showXYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_XZ_PLANE, skeletonViewportWidget->showXZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_YZ_PLANE, skeletonViewportWidget->showYZPlaneCheckBox.isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, skeletonViewportWidget->rotateAroundActiveNodeCheckBox.isChecked());

    settings.setValue(VP_TAB_INDEX, tabs->currentIndex());

    settings.endGroup();
}

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
#include "appearancewidget.h"

#include "GuiConstants.h"
#include "skeleton/skeletonizer.h"
#include "viewer.h"

#include <QSettings>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>

AppearanceWidget::AppearanceWidget(QWidget *parent) : QDialog(parent) {
    setWindowIcon(QIcon(":/resources/icons/view-list-icons-symbolic.png"));
    setWindowTitle("Appearance Settings");
    tabs.addTab(&skeletonTab, "Skeleton");
    tabs.addTab(&datasetAndSegmentationTab, "Dataset && Segmentation");
    tabs.addTab(&viewportTab, "Viewports");

    mainLayout.addWidget(&tabs);
    setLayout(&mainLayout);

    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
}

void AppearanceWidget::loadSettings() {
    int width, height, x, y;
    bool visible;

    QSettings settings;
    settings.beginGroup(APPEARANCE_WIDGET);
    width = settings.value(WIDTH, this->width()).toInt();
    height = settings.value(HEIGHT, this->height()).toInt();
    if(settings.value(POS_X).isNull() || settings.value(POS_Y).isNull()) {
        x = QApplication::desktop()->screen()->rect().topRight().x() - this->width() - 20;
        y = QApplication::desktop()->screen()->rect().topRight().y() + this->height();
    }
    else {
        x = settings.value(POS_X).toInt();
        y = settings.value(POS_Y).toInt();
    }
    visible = settings.value(VISIBLE, false).toBool();
    // skeleton visualization
    skeletonTab.lightEffectsCheck.setChecked(settings.value(LIGHT_EFFECTS, true).toBool());
    skeletonTab.highlightActiveTreeCheck.setChecked(settings.value(HIGHLIGHT_ACTIVE_TREE, true).toBool());
    skeletonTab.allNodeIDsCheck.setChecked(settings.value(SHOW_ALL_NODE_ID, false).toBool());
    skeletonTab.edgeNodeRatioSpin.setValue(settings.value(EDGE_TO_NODE_RADIUS, 1.5).toDouble());
    skeletonTab.renderQualitySpin.setValue(settings.value(RENDERING_QUALITY, 7).toInt());
    skeletonTab.wholeSkeletonRadio.setChecked(settings.value(WHOLE_SKELETON, true).toBool());
    skeletonTab.selectedTreesRadio.setChecked(settings.value(ONLY_SELECTED_TREES, false).toBool());
    skeletonTab.skeletonInOrthoVPsCheck.setChecked(settings.value(SHOW_SKELETON_ORTHOVPS, true).toBool());
    skeletonTab.skeletonIn3DVPCheck.setChecked(settings.value(SHOW_SKELETON_SKELVP, true).toBool());
    skeletonTab.overrideNodeRadiusCheck.setChecked(settings.value(OVERRIDE_NODES_RADIUS_CHECKED, false).toBool());
    skeletonTab.nodeRadiusSpin.setEnabled(state->skeletonState->overrideNodeRadiusBool);
    skeletonTab.nodeRadiusSpin.setValue(settings.value(OVERRIDE_NODES_RADIUS_VALUE, 1.5).toDouble());
    skeletonTab.edgeNodeRatioSpin.setValue(settings.value(EDGE_TO_NODE_RADIUS, 0.5).toFloat());
    skeletonTab.highlightIntersectionsCheck.setChecked(settings.value(HIGHLIGHT_INTERSECTIONS, false).toBool());
    skeletonTab.depthCutoffSpin.setValue(settings.value(DEPTH_CUTOFF, 5.).toDouble());
    skeletonTab.nodeCommentsCheck.setChecked(settings.value(SHOW_NODE_COMMENTS, false).toBool());
    skeletonTab.lutFilePath = settings.value(TREE_LUT_FILE, "").toString();
    //it’s impotant to populate the checkbox after loading the path-string, because emitted signals depend on the lut // TODO VP settings: is that true?
    skeletonTab.ownTreeColorsCheck.setChecked(settings.value(TREE_LUT_FILE_USED, false).toBool());

    //dataset & segmentation
    const auto linearFiltering = settings.value(DATASET_LINEAR_FILTERING, true).toBool();
    datasetAndSegmentationTab.datasetLinearFilteringCheckBox.setChecked(linearFiltering);
    datasetAndSegmentationTab.datasetLinearFilteringCheckBox.clicked(linearFiltering);
    datasetAndSegmentationTab.lutFilePath = settings.value(DATASET_LUT_FILE, "").toString();
    // again, load the path-string first, before populating the checkbox
    const auto useDatasetLut = settings.value(DATASET_LUT_FILE_USED, false).toBool();
    datasetAndSegmentationTab.useOwnDatasetColorsCheckBox.setChecked(useDatasetLut);
    datasetAndSegmentationTab.useOwnDatasetColorsCheckBox.clicked(useDatasetLut); //reload LUT

    const auto luminanceBias = settings.value(BIAS, 0).toInt();
    datasetAndSegmentationTab.biasSpinBox.setValue(luminanceBias);
    datasetAndSegmentationTab.biasSpinBox.valueChanged(luminanceBias);

    const auto luminanceRangeDelta = settings.value(RANGE_DELTA, 255).toInt();
    datasetAndSegmentationTab.rangeDeltaSpinBox.setValue(luminanceRangeDelta);
    datasetAndSegmentationTab.rangeDeltaSpinBox.valueChanged(luminanceRangeDelta);

    const auto segmentationOverlayAlpha = settings.value(SEGMENTATION_OVERLAY_ALPHA, 37).toInt();
    datasetAndSegmentationTab.segmentationOverlaySlider.setValue(segmentationOverlayAlpha);
    datasetAndSegmentationTab.segmentationOverlaySlider.valueChanged(segmentationOverlayAlpha);

    datasetAndSegmentationTab.volumeRenderCheckBox.setChecked(settings.value(RENDER_VOLUME, false).toBool());
    datasetAndSegmentationTab.volumeOpaquenessSpinBox.setValue(settings.value(VOLUME_ALPHA, 37).toInt());

    Segmentation::singleton().volume_background_color.setRgba(settings.value(VOLUME_BACKGROUND_COLOR, QColor(Qt::darkGray)).toUInt());
    datasetAndSegmentationTab.volumeColorButton.setStyleSheet("background-color: " + Segmentation::singleton().volume_background_color.name() + ";");

    // vp settings
    viewportTab.showScalebarCheckBox.setChecked(settings.value(SHOW_SCALEBAR, false).toBool());

    viewportTab.showVPDecorationCheckBox.setChecked(settings.value(SHOW_VP_DECORATION, true).toBool());

    const auto drawVPCrosshairs = settings.value(DRAW_INTERSECTIONS_CROSSHAIRS, true).toBool();
    viewportTab.drawIntersectionsCrossHairCheckBox.setChecked(drawVPCrosshairs);
    viewportTab.drawIntersectionsCrossHairCheckBox.clicked(drawVPCrosshairs);

    const auto xyplane = settings.value(SHOW_XY_PLANE, true).toBool();
    viewportTab.showXYPlaneCheckBox.setChecked(xyplane);
    viewportTab.showXYPlaneCheckBox.clicked(xyplane);

    const auto xzplane = settings.value(SHOW_XZ_PLANE, true).toBool();
    viewportTab.showXZPlaneCheckBox.setChecked(xzplane);
    viewportTab.showXZPlaneCheckBox.clicked(xzplane);

    const auto yzplane = settings.value(SHOW_YZ_PLANE, true).toBool();
    viewportTab.showYZPlaneCheckBox.setChecked(yzplane);
    viewportTab.showYZPlaneCheckBox.clicked(yzplane);

    const auto showPhysicalBoundaries = settings.value(SHOW_PHYSICAL_BOUNDARIES, false).toBool();
    viewportTab.boundariesPixelRadioBtn.setChecked(!showPhysicalBoundaries);
    viewportTab.boundariesPhysicalRadioBtn.setChecked(showPhysicalBoundaries);

    const auto rotateAroundActiveNode = settings.value(ROTATE_AROUND_ACTIVE_NODE, true).toBool();
    viewportTab.rotateAroundActiveNodeCheckBox.setChecked(rotateAroundActiveNode);
    viewportTab.rotateAroundActiveNodeCheckBox.clicked(rotateAroundActiveNode);

    tabs.setCurrentIndex(settings.value(VP_TAB_INDEX, 0).toInt());

    settings.endGroup();
    setVisible(visible);
    setGeometry(x, y, width, height);
}

void AppearanceWidget::saveSettings() {
    QSettings settings;
    settings.beginGroup(APPEARANCE_WIDGET);
    settings.setValue(WIDTH, geometry().width());
    settings.setValue(HEIGHT, geometry().height());
    settings.setValue(POS_X, geometry().x());
    settings.setValue(POS_Y, geometry().y());
    settings.setValue(VISIBLE, isVisible());
    // skeleton
    settings.setValue(LIGHT_EFFECTS, skeletonTab.lightEffectsCheck.isChecked());
    settings.setValue(HIGHLIGHT_ACTIVE_TREE, skeletonTab.highlightActiveTreeCheck.isChecked());
    settings.setValue(HIGHLIGHT_INTERSECTIONS, skeletonTab.highlightIntersectionsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE_USED, skeletonTab.ownTreeColorsCheck.isChecked());
    settings.setValue(TREE_LUT_FILE, skeletonTab.lutFilePath);
    settings.setValue(DEPTH_CUTOFF, skeletonTab.depthCutoffSpin.value());
    settings.setValue(SHOW_ALL_NODE_ID, skeletonTab.allNodeIDsCheck.isChecked());
    settings.setValue(EDGE_TO_NODE_RADIUS, skeletonTab.edgeNodeRatioSpin.value());
    settings.setValue(RENDERING_QUALITY, skeletonTab.renderQualitySpin.value());
    settings.setValue(WHOLE_SKELETON, skeletonTab.wholeSkeletonRadio.isChecked());
    settings.setValue(ONLY_SELECTED_TREES, skeletonTab.selectedTreesRadio.isChecked());
    settings.setValue(SHOW_SKELETON_ORTHOVPS, skeletonTab.skeletonInOrthoVPsCheck.isChecked());
    settings.setValue(SHOW_SKELETON_SKELVP, skeletonTab.skeletonIn3DVPCheck.isChecked());
    settings.setValue(OVERRIDE_NODES_RADIUS_CHECKED, skeletonTab.overrideNodeRadiusCheck.isChecked());
    settings.setValue(OVERRIDE_NODES_RADIUS_VALUE, skeletonTab.nodeRadiusSpin.value());
    settings.setValue(SHOW_NODE_COMMENTS, skeletonTab.nodeCommentsCheck.isChecked());
    // dataset & segmentation
    settings.setValue(DATASET_LINEAR_FILTERING, datasetAndSegmentationTab.datasetLinearFilteringCheckBox.isChecked());
    settings.setValue(BIAS, datasetAndSegmentationTab.biasSpinBox.value());
    settings.setValue(RANGE_DELTA, datasetAndSegmentationTab.rangeDeltaSpinBox.value());
    settings.setValue(SEGMENTATION_OVERLAY_ALPHA, datasetAndSegmentationTab.segmentationOverlaySlider.value());
    settings.setValue(DATASET_LUT_FILE, datasetAndSegmentationTab.lutFilePath);
    settings.setValue(DATASET_LUT_FILE_USED, datasetAndSegmentationTab.useOwnDatasetColorsCheckBox.isChecked());
    settings.setValue(RENDER_VOLUME, datasetAndSegmentationTab.volumeRenderCheckBox.isChecked());
    settings.setValue(VOLUME_ALPHA, datasetAndSegmentationTab.volumeOpaquenessSpinBox.value());
    settings.setValue(VOLUME_BACKGROUND_COLOR, Segmentation::singleton().volume_background_color.rgba());
    // viewports
    settings.setValue(SHOW_SCALEBAR, viewportTab.showScalebarCheckBox.isChecked());
    settings.setValue(SHOW_VP_DECORATION, viewportTab.showVPDecorationCheckBox.isChecked());
    settings.setValue(DRAW_INTERSECTIONS_CROSSHAIRS, viewportTab.drawIntersectionsCrossHairCheckBox.isChecked());
    settings.setValue(SHOW_XY_PLANE, viewportTab.showXYPlaneCheckBox.isChecked());
    settings.setValue(SHOW_XZ_PLANE, viewportTab.showXZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_YZ_PLANE, viewportTab.showYZPlaneCheckBox.isChecked());
    settings.setValue(SHOW_PHYSICAL_BOUNDARIES, viewportTab.boundariesPhysicalRadioBtn.isChecked());
    settings.setValue(ROTATE_AROUND_ACTIVE_NODE, viewportTab.rotateAroundActiveNodeCheckBox.isChecked());

    settings.setValue(VP_TAB_INDEX, tabs.currentIndex());

    settings.endGroup();
}

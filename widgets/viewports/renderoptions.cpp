#include "renderoptions.h"
#include "dataset.h"
#include "stateInfo.h"
#include "viewer.h"

RenderOptions::RenderOptions()
    : drawCrosshairs(state->viewerState->drawVPCrosshairs && state->viewerState->showOnlyRawData == false)
        , drawMesh{state->viewerState->showOnlyRawData == false}
        , drawOverlay(Segmentation::singleton().enabled && state->viewerState->showOnlyRawData == false)
        , drawSkeleton{state->viewerState->showOnlyRawData == false}
        , highlightActiveNode(state->viewerState->cumDistRenderThres <= 19.f)// no active node halo in lines and points mode
{}

RenderOptions RenderOptions::nodePickingRenderOptions(RenderOptions::SelectionPass pass) {
    RenderOptions options;
    options.drawBoundaryAxes = options.drawBoundaryBox = options.drawCrosshairs = options.drawOverlay = options.drawMesh = false;
    options.drawViewportPlanes = options.highlightActiveNode = options.highlightSelection = false;
    options.nodePicking = true;
    options.selectionPass = pass;
    return options;
}

RenderOptions RenderOptions::snapshotRenderOptions(const bool drawBoundaryAxes, const bool drawBoundaryBox, const bool drawOverlay, const bool drawMesh, const bool drawSkeleton, const bool drawViewportPlanes) {
    RenderOptions options;
    options.drawBoundaryAxes = drawBoundaryAxes;
    options.drawBoundaryBox = drawBoundaryBox;
    options.drawCrosshairs = false;
    options.drawOverlay = drawOverlay;
    options.drawMesh = drawMesh;
    options.drawSkeleton = drawSkeleton;
    options.drawViewportPlanes = drawViewportPlanes;
    options.enableLoddingAndLinesAndPoints = false;
    options.enableTextScaling = true;
    options.highlightActiveNode = false;
    options.highlightSelection = false;
    options.vp3dSliceBoundaries = false;
    options.vp3dSliceIntersections = false;
    return options;
}

bool RenderOptions::useLinesAndPoints(const float radius, const float smallestVisibleSize) const {
    const auto nodesVisible = radius * 2 >= smallestVisibleSize;
    const auto alwaysLinesAndPoints = state->viewerState->cumDistRenderThres > 19.f && enableLoddingAndLinesAndPoints;
    const auto switchDynamically = !alwaysLinesAndPoints && enableLoddingAndLinesAndPoints;
    const auto dynamicLinesAndPoints = switchDynamically && !nodesVisible;
    return alwaysLinesAndPoints || dynamicLinesAndPoints;
}

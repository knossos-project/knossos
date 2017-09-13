#ifndef RENDEROPTIONS_H
#define RENDEROPTIONS_H

struct RenderOptions {
    enum class SelectionPass { NoSelection, NodeID0_24Bits, NodeID24_48Bits, NodeID48_64Bits };
    RenderOptions();
    static RenderOptions nodePickingRenderOptions(SelectionPass pass);
    static RenderOptions meshPickingRenderOptions();
    static RenderOptions snapshotRenderOptions(const bool drawBoundaryAxes, const bool drawBoundaryBox, const bool drawOverlay, const bool drawMesh, const bool drawSkeleton, const bool drawViewportPlanes);

    bool useLinesAndPoints(const float radius, const float smallestVisibleSize) const;

    bool drawBoundaryAxes{true};
    bool drawBoundaryBox{true};
    bool drawCrosshairs{true};
    bool drawOverlay{true};
    bool drawMesh{true};
    bool drawSkeleton{true};
    bool drawViewportPlanes{true};
    bool enableLoddingAndLinesAndPoints{true};
    bool enableTextScaling{false};
    bool highlightActiveNode{true};
    bool highlightSelection{true};
    bool nodePicking{false};
    bool meshPicking{false};
    SelectionPass selectionPass{SelectionPass::NoSelection};
};

#endif // RENDEROPTIONS_H

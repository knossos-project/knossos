#include "vpsliceplaneviewportwidget.h"

VPSlicePlaneViewportWidget::VPSlicePlaneViewportWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QGridLayout *gridLayout = new QGridLayout();

    skeletonOverlayLabel = new QLabel("Skeleton Overlay");
    voxelFilteringLabel = new QLabel("Voxel Filtering");

    enableOverlayCheckBox = new QCheckBox("Enable Overlay");
    highlightIntersectionsCheckBox = new QCheckBox("Highlight Intersections");
    datasetLinearFilteringCheckBox = new QCheckBox("Dataset Linear Filtering");

    colorLookupTablesLabel = new QLabel("Color Lookup Tables");
    datasetDynamicRangeLabel = new QLabel("Dataset Dynamic Range");

    depthCutoffLabel = new QLabel("Depth Cutoff");
    depthCutoffSpinBox = new QDoubleSpinBox();

    useOwnDatasetColorsCheckBox = new QCheckBox("Use Own Dataset Colors");
    useOwnTreeColorsCheckBox = new QCheckBox("Use Own Tree Colors");
    useOwnDatasetColorsButton = new QPushButton("Load");
    useOwnTreeColorButton = new QPushButton("Load");

    biasLabel = new QLabel("Bias");
    biasSpinBox = new QSpinBox();
    biasSlider = new QSlider(Qt::Horizontal);

    rangeDeltaLabel = new QLabel("Range Delta");
    rangeDeltaSlider = new QSlider(Qt::Horizontal);
    rangeDeltaSpinBox = new QSpinBox();

    objectIDOverlayLabel = new QLabel("Object ID Overlay");
    viewportObjectsLabel = new QLabel("Viewport Objects");

    enableColorOverlayCheckBox = new QCheckBox("Enable Color Overlay");
    drawIntersectionsCrossHairCheckBox = new QCheckBox("Draw Intersections Crosshairs");
    showViewPortsSizeCheckBox = new QCheckBox("Show Viewport Size");

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame *line2 = new QFrame();
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QFrame *line3 = new QFrame();
    line3->setFrameShape(QFrame::HLine);
    line3->setFrameShadow(QFrame::Sunken);

    QFrame *line4 = new QFrame();
    line4->setFrameShape(QFrame::HLine);
    line4->setFrameShadow(QFrame::Sunken);

    QFrame *line5 = new QFrame();
    line5->setFrameShape(QFrame::HLine);
    line5->setFrameShadow(QFrame::Sunken);

    QFrame *line6 = new QFrame();
    line6->setFrameShape(QFrame::HLine);
    line6->setFrameShadow(QFrame::Sunken);

    gridLayout->addWidget(skeletonOverlayLabel, 0, 0);
    gridLayout->addWidget(voxelFilteringLabel, 0, 3);
    gridLayout->addWidget(line, 1, 0);
    gridLayout->addWidget(line2, 1, 3);
    gridLayout->addWidget(enableOverlayCheckBox, 2, 0);
    gridLayout->addWidget(datasetLinearFilteringCheckBox, 2, 3);
    gridLayout->addWidget(highlightIntersectionsCheckBox, 3, 0);
    gridLayout->addWidget(depthCutoffLabel, 4, 0);
    gridLayout->addWidget(depthCutoffSpinBox, 4, 1);
    gridLayout->addWidget(colorLookupTablesLabel, 7, 0);
    gridLayout->addWidget(datasetDynamicRangeLabel, 7, 3);
    gridLayout->addWidget(line3, 8, 0);
    gridLayout->addWidget(line4, 8, 3);
    gridLayout->addWidget(useOwnDatasetColorsCheckBox, 9, 0);
    gridLayout->addWidget(useOwnDatasetColorsButton, 9, 1);
    gridLayout->addWidget(biasLabel, 9, 3);
    gridLayout->addWidget(biasSlider, 9, 4);
    gridLayout->addWidget(biasSpinBox, 9, 5);
    gridLayout->addWidget(useOwnTreeColorsCheckBox, 10, 0);
    gridLayout->addWidget(useOwnTreeColorButton, 10, 1);
    gridLayout->addWidget(rangeDeltaLabel, 10, 3);
    gridLayout->addWidget(rangeDeltaSlider, 10, 4);
    gridLayout->addWidget(rangeDeltaSpinBox, 10, 5);
    gridLayout->addWidget(objectIDOverlayLabel, 11, 0);
    gridLayout->addWidget(viewportObjectsLabel, 11, 3);
    gridLayout->addWidget(line5, 12, 0);
    gridLayout->addWidget(line6, 12, 3);
    gridLayout->addWidget(enableColorOverlayCheckBox, 13, 0);
    gridLayout->addWidget(drawIntersectionsCrossHairCheckBox, 13, 3);
    gridLayout->addWidget(showViewPortsSizeCheckBox, 14, 3);

    mainLayout->addLayout(gridLayout);
    setLayout(mainLayout);
}

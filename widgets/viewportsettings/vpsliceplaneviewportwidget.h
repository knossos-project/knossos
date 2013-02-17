#ifndef VPSLICEPLANEVIEWPORTWIDGET_H
#define VPSLICEPLANEVIEWPORTWIDGET_H

#include <QWidget>
class QLabel;
class QCheckBox;
class QDoubleSpinBox;
class QPushButton;
class QSpinBox;
class QSlider;
class VPSlicePlaneViewportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VPSlicePlaneViewportWidget(QWidget *parent = 0);
    
signals:
    
public slots:
    
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

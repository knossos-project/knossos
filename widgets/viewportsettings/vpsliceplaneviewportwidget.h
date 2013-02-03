#ifndef VPSLICEPLANEVIEWPORTWIDGET_H
#define VPSLICEPLANEVIEWPORTWIDGET_H

#include <QWidget>
#include <Qlabel>
#include <QFrame>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QGridLayout>


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

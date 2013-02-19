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
    void loadSettings();
signals:
    
public slots:
    void enableOverlayChecked(bool on);
    void datasetLinearFilteringChecked(bool on);
    void hightlightIntersectionsChecked(bool on);
    void depthCutoffChanged(double value);
    void useOwnDatasetColorsChecked(bool on);
    void useOwnTreeColorsChecked(bool on);
    void useOwnDatasetColorsButtonClicked();
    void useOwnTreeColorButtonClicked();
    void biasSliderMoved(int value);
    void biasChanged(int value);
    void rangeDeltaSliderMoved(int value);
    void rangeDeltaChanged(int value);
    void enableColorOverlayChecked(bool on);
    void drawIntersectionsCrossHairChecked(bool on);
    void showViewPortsSizeChecked(bool on);

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

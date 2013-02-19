#ifndef VPSKELETONVIEWPORTWIDGET_H
#define VPSKELETONVIEWPORTWIDGET_H

#include <QWidget>

class QLabel;
class QCheckBox;
class QRadioButton;
class VPSkeletonViewportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VPSkeletonViewportWidget(QWidget *parent = 0);
    void loadSettings();
signals:
    
public slots:
    void showXYPlaneChecked(bool on);
    void showXZPlaneChecked(bool on);
    void showYZPlaneChecked(bool on);
    void wholeSkeletonSelected();
    void onlyCurrentCubeSelected();
    void onlyActiveTreeSelected();
    void hideSkeletonSelected();
    void rotateAroundActiveNodeChecked(bool on);
protected:
    QLabel *datasetVisualizationLabel, *skeletonDisplayModesLabel;
    QCheckBox *showXYPlaneCheckBox, *showXZPlaneCheckBox, *showYZPlaneCheckBox;
    QRadioButton *wholeSkeletonButton, *onlyCurrentCubeButton, *onlyActiveTreeButton, *hideSkeletonButton;
    QLabel *view3dlabel;
    QCheckBox *rotateAroundActiveNodeCheckBox;
    
};

#endif // VPSKELETONVIEWPORTWIDGET_H
